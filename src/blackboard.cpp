#include "behaviortree_cpp/blackboard.h"
#include <unordered_set>
#include "behaviortree_cpp/json_export.h"

namespace BT
{

bool IsPrivateKey(StringView str)
{
  return str.size() >= 1 && str.data()[0] == '_';
}

void Blackboard::enableAutoRemapping(bool remapping)
{
  autoremapping_ = remapping;
}

AnyPtrLocked Blackboard::getAnyLocked(const std::string& key)
{
  if(auto entry = getEntry(key))
  {
    return AnyPtrLocked(&entry->value, &entry->entry_mutex);
  }
  return {};
}

AnyPtrLocked Blackboard::getAnyLocked(const std::string& key) const
{
  if(auto entry = getEntry(key))
  {
    return AnyPtrLocked(&entry->value, const_cast<std::mutex*>(&entry->entry_mutex));
  }
  return {};
}

const Any* Blackboard::getAny(const std::string& key) const
{
  return getAnyLocked(key).get();
}

Any* Blackboard::getAny(const std::string& key)
{
  return const_cast<Any*>(getAnyLocked(key).get());
}

const std::shared_ptr<Blackboard::Entry>
Blackboard::getEntry(const std::string& key) const
{
  // special syntax: "@" will always refer to the root BB
  if(StartWith(key, '@'))
  {
    return rootBlackboard()->getEntry(key.substr(1, key.size() - 1));
  }

  std::unique_lock<std::mutex> lock(mutex_);
  auto it = storage_.find(key);
  if(it != storage_.end())
  {
    return it->second;
  }
  // not found. Try autoremapping
  if(auto parent = parent_bb_.lock())
  {
    auto remap_it = internal_to_external_.find(key);
    if(remap_it != internal_to_external_.cend())
    {
      auto const& new_key = remap_it->second;
      return parent->getEntry(new_key);
    }
    if(autoremapping_ && !IsPrivateKey(key))
    {
      return parent->getEntry(key);
    }
  }
  return {};
}

std::shared_ptr<Blackboard::Entry> Blackboard::getEntry(const std::string& key)
{
  return static_cast<const Blackboard&>(*this).getEntry(key);
}

const TypeInfo* Blackboard::entryInfo(const std::string& key)
{
  auto entry = getEntry(key);
  return (!entry) ? nullptr : &(entry->info);
}

void Blackboard::addSubtreeRemapping(StringView internal, StringView external)
{
  internal_to_external_.insert(
      { static_cast<std::string>(internal), static_cast<std::string>(external) });
}

void Blackboard::debugMessage() const
{
  for(const auto& [key, entry] : storage_)
  {
    auto port_type = entry->info.type();
    if(port_type == typeid(void))
    {
      port_type = entry->value.type();
    }

    std::cout << key << " (" << BT::demangle(port_type) << ")" << std::endl;
  }

  for(const auto& [from, to] : internal_to_external_)
  {
    std::cout << "[" << from << "] remapped to port of parent tree [" << to << "]"
              << std::endl;
    continue;
  }
}

std::vector<StringView> Blackboard::getKeys() const
{
  if(storage_.empty())
  {
    return {};
  }
  std::vector<StringView> out;
  out.reserve(storage_.size());
  for(const auto& entry_it : storage_)
  {
    out.push_back(entry_it.first);
  }
  return out;
}

void Blackboard::clear()
{
  std::unique_lock<std::mutex> lock(mutex_);
  storage_.clear();
}

std::recursive_mutex& Blackboard::entryMutex() const
{
  return entry_mutex_;
}

void Blackboard::createEntry(const std::string& key, const TypeInfo& info)
{
  if(StartWith(key, '@'))
  {
    rootBlackboard()->createEntryImpl(key.substr(1, key.size() - 1), info);
  }
  else
  {
    createEntryImpl(key, info);
  }
}

void Blackboard::cloneInto(Blackboard& dst) const
{
  std::unique_lock lk1(mutex_);
  std::unique_lock lk2(dst.mutex_);

  // keys that are not updated must be removed.
  std::unordered_set<std::string> keys_to_remove;
  auto& dst_storage = dst.storage_;
  for(const auto& [key, _] : dst_storage)
  {
    keys_to_remove.insert(key);
  }

  // update or create entries in dst_storage
  for(const auto& [src_key, src_entry] : storage_)
  {
    keys_to_remove.erase(src_key);

    auto it = dst_storage.find(src_key);
    if(it != dst_storage.end())
    {
      // overwite
      auto& dst_entry = it->second;
      dst_entry->string_converter = src_entry->string_converter;
      dst_entry->value = src_entry->value;
      dst_entry->info = src_entry->info;
      dst_entry->sequence_id++;
      dst_entry->stamp = std::chrono::steady_clock::now().time_since_epoch();
    }
    else
    {
      // create new
      auto new_entry = std::make_shared<Entry>(src_entry->info);
      new_entry->value = src_entry->value;
      new_entry->string_converter = src_entry->string_converter;
      dst_storage.insert({ src_key, new_entry });
    }
  }

  for(const auto& key : keys_to_remove)
  {
    dst_storage.erase(key);
  }
}

Blackboard::Ptr Blackboard::parent()
{
  if(auto parent = parent_bb_.lock())
  {
    return parent;
  }
  return {};
}

std::shared_ptr<Blackboard::Entry> Blackboard::createEntryImpl(const std::string& key,
                                                               const TypeInfo& info)
{
  std::unique_lock<std::mutex> lock(mutex_);
  // This function might be called recursively, when we do remapping, because we move
  // to the top scope to find already existing  entries

  // search if exists already
  auto storage_it = storage_.find(key);
  if(storage_it != storage_.end())
  {
    const auto& prev_info = storage_it->second->info;
    auto prev_type_demangled = BT::demangle(prev_info.type());
    // Allow mismatch if going from vector -> vector<Any>.
    bool previous_is_vector = BT::isVector(prev_type_demangled);
    bool new_is_vector_any = info.type() == typeid(std::vector<Any>);

    if(prev_info.type() != info.type() && prev_info.isStronglyTyped() &&
       info.isStronglyTyped() && !(previous_is_vector && new_is_vector_any))
    {
      auto msg = StrCat("Blackboard entry [", key,
                        "]: once declared, the type of a port"
                        " shall not change. Previously declared type [",
                        prev_type_demangled, "], current type [",
                        BT::demangle(info.type()), "]");

      throw LogicError(msg);
    }
    return storage_it->second;
  }

  // manual remapping first
  auto remapping_it = internal_to_external_.find(key);
  if(remapping_it != internal_to_external_.end())
  {
    const auto& remapped_key = remapping_it->second;
    if(auto parent = parent_bb_.lock())
    {
      return parent->createEntryImpl(remapped_key, info);
    }
    throw RuntimeError("Missing parent blackboard");
  }
  // autoremapping second (excluding private keys)
  if(autoremapping_ && !IsPrivateKey(key))
  {
    if(auto parent = parent_bb_.lock())
    {
      return parent->createEntryImpl(key, info);
    }
    throw RuntimeError("Missing parent blackboard");
  }
  // not remapped, not found. Create locally.

  auto entry = std::make_shared<Entry>(info);
  // even if empty, let's assign to it a default type
  entry->value = Any(info.type());
  storage_.insert({ key, entry });
  return entry;
}

nlohmann::json ExportBlackboardToJSON(const Blackboard& blackboard)
{
  nlohmann::json dest;
  for(auto entry_name : blackboard.getKeys())
  {
    std::string name(entry_name);
    if(auto any_ref = blackboard.getAnyLocked(name))
    {
      if(auto any_ptr = any_ref.get())
      {
        JsonExporter::get().toJson(*any_ptr, dest[name]);
      }
    }
  }
  return dest;
}

void ImportBlackboardFromJSON(const nlohmann::json& json, Blackboard& blackboard)
{
  for(auto it = json.begin(); it != json.end(); ++it)
  {
    if(auto res = JsonExporter::get().fromJson(it.value()))
    {
      auto entry = blackboard.getEntry(it.key());
      if(!entry)
      {
        blackboard.createEntry(it.key(), res->second);
        entry = blackboard.getEntry(it.key());
      }
      entry->value = res->first;
    }
  }
}

Blackboard::Entry& Blackboard::Entry::operator=(const Entry& other)
{
  value = other.value;
  info = other.info;
  string_converter = other.string_converter;
  sequence_id = other.sequence_id;
  stamp = other.stamp;
  return *this;
}

Blackboard* BT::Blackboard::rootBlackboard()
{
  auto bb = static_cast<const Blackboard&>(*this).rootBlackboard();
  return const_cast<Blackboard*>(bb);
}

const Blackboard* BT::Blackboard::rootBlackboard() const
{
  const Blackboard* bb = this;
  Blackboard::Ptr prev = parent_bb_.lock();
  while(prev)
  {
    bb = prev.get();
    prev = bb->parent_bb_.lock();
  }
  return bb;
}

}  // namespace BT
