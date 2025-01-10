#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <regex>

#include "behaviortree_cpp/basic_types.h"
#include "behaviortree_cpp/contrib/json.hpp"
#include "behaviortree_cpp/utils/safe_any.hpp"
#include "behaviortree_cpp/exceptions.h"
#include "behaviortree_cpp/utils/locked_reference.hpp"

namespace BT
{

/// This type contains a pointer to Any, protected
/// with a locked mutex as long as the object is in scope
using AnyPtrLocked = LockedPtr<Any>;

template <typename T>
struct StampedValue
{
  T value;
  Timestamp stamp;
};

// Helper trait to check if templated type is a std::vector
template <typename T>
struct is_vector : std::false_type
{
};

template <typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type
{
};

// Helper function to check if a demangled type string is a std::vector<..>
inline bool isVector(const std::string& type_name)
{
  return std::regex_match(type_name, std::regex(R"(^std::vector<.*>$)"));
}

/**
 * @brief The Blackboard is the mechanism used by BehaviorTrees to exchange
 * typed data.
 */
class Blackboard
{
public:
  using Ptr = std::shared_ptr<Blackboard>;

protected:
  // This is intentionally protected. Use Blackboard::create instead
  Blackboard(Blackboard::Ptr parent) : parent_bb_(parent)
  {}

public:
  struct Entry
  {
    Any value;
    TypeInfo info;
    StringConverter string_converter;
    mutable std::mutex entry_mutex;

    uint64_t sequence_id = 0;
    // timestamp since epoch
    std::chrono::nanoseconds stamp = std::chrono::nanoseconds{ 0 };

    Entry(const TypeInfo& _info) : info(_info)
    {}

    Entry& operator=(const Entry& other);
  };

  /** Use this static method to create an instance of the BlackBoard
    *   to share among all your NodeTrees.
    */
  static Blackboard::Ptr create(Blackboard::Ptr parent = {})
  {
    return std::shared_ptr<Blackboard>(new Blackboard(parent));
  }

  virtual ~Blackboard() = default;

  void enableAutoRemapping(bool remapping);

  [[nodiscard]] const std::shared_ptr<Entry> getEntry(const std::string& key) const;

  [[nodiscard]] std::shared_ptr<Blackboard::Entry> getEntry(const std::string& key);

  [[nodiscard]] AnyPtrLocked getAnyLocked(const std::string& key);

  [[nodiscard]] AnyPtrLocked getAnyLocked(const std::string& key) const;

  [[deprecated("Use getAnyLocked instead")]] const Any*
  getAny(const std::string& key) const;

  [[deprecated("Use getAnyLocked instead")]] Any* getAny(const std::string& key);

  /** Return true if the entry with the given key was found.
   *  Note that this method may throw an exception if the cast to T failed.
   */
  template <typename T>
  [[nodiscard]] bool get(const std::string& key, T& value) const;

  template <typename T>
  [[nodiscard]] Expected<Timestamp> getStamped(const std::string& key, T& value) const;

  /**
   * Version of get() that throws if it fails.
   */
  template <typename T>
  [[nodiscard]] T get(const std::string& key) const;

  template <typename T>
  [[nodiscard]] Expected<StampedValue<T>> getStamped(const std::string& key) const;

  /// Update the entry with the given key
  template <typename T>
  void set(const std::string& key, const T& value);

  void unset(const std::string& key);

  [[nodiscard]] const TypeInfo* entryInfo(const std::string& key);

  void addSubtreeRemapping(StringView internal, StringView external);

  void debugMessage() const;

  [[nodiscard]] std::vector<StringView> getKeys() const;

  [[deprecated("This command is unsafe. Consider using Backup/Restore instead")]] void
  clear();

  [[deprecated("Use getAnyLocked to access safely an Entry")]] std::recursive_mutex&
  entryMutex() const;

  void createEntry(const std::string& key, const TypeInfo& info);

  /**
   * @brief cloneInto copies the values of the entries
   * into another blackboard.  Known limitations:
   *
   * - it doesn't update the remapping in dst
   * - it doesn't change the parent blackboard os dst
   *
   * @param dst destination, i.e. blackboard to be updated
   */
  void cloneInto(Blackboard& dst) const;

  Blackboard::Ptr parent();

  // recursively look for parent Blackboard, until you find the root
  Blackboard* rootBlackboard();

  const Blackboard* rootBlackboard() const;

private:
  mutable std::mutex mutex_;
  mutable std::recursive_mutex entry_mutex_;
  std::unordered_map<std::string, std::shared_ptr<Entry>> storage_;
  std::weak_ptr<Blackboard> parent_bb_;
  std::unordered_map<std::string, std::string> internal_to_external_;

  std::shared_ptr<Entry> createEntryImpl(const std::string& key, const TypeInfo& info);

  bool autoremapping_ = false;
};

/**
 * @brief ExportBlackboardToJSON will create a JSON
 * that contains the current values of the blackboard.
 * Complex types must be registered with JsonExporter::get()
 */
nlohmann::json ExportBlackboardToJSON(const Blackboard& blackboard);

/**
 * @brief ImportBlackboardFromJSON will append elements to the blackboard,
 * using the values parsed from the JSON file created using ExportBlackboardToJSON.
 * Complex types must be registered with JsonExporter::get()
 */
void ImportBlackboardFromJSON(const nlohmann::json& json, Blackboard& blackboard);

//------------------------------------------------------

template <typename T>
inline T Blackboard::get(const std::string& key) const
{
  if(auto any_ref = getAnyLocked(key))
  {
    const auto& any = any_ref.get();
    if(any->empty())
    {
      throw RuntimeError("Blackboard::get() error. Entry [", key,
                         "] hasn't been initialized, yet");
    }
    return any_ref.get()->cast<T>();
  }
  throw RuntimeError("Blackboard::get() error. Missing key [", key, "]");
}

inline void Blackboard::unset(const std::string& key)
{
  std::unique_lock lock(mutex_);

  // check local storage
  auto it = storage_.find(key);
  if(it == storage_.end())
  {
    // No entry, nothing to do.
    return;
  }

  storage_.erase(it);
}

template <typename T>
inline void Blackboard::set(const std::string& key, const T& value)
{
  if(StartWith(key, '@'))
  {
    rootBlackboard()->set(key.substr(1, key.size() - 1), value);
    return;
  }
  std::unique_lock lock(mutex_);

  // check local storage
  auto it = storage_.find(key);
  if(it == storage_.end())
  {
    // create a new entry
    Any new_value(value);
    lock.unlock();
    std::shared_ptr<Blackboard::Entry> entry;
    // if a new generic port is created with a string, it's type should be AnyTypeAllowed
    if constexpr(std::is_same_v<std::string, T>)
    {
      entry = createEntryImpl(key, PortInfo(PortDirection::INOUT));
    }
    else
    {
      PortInfo new_port(PortDirection::INOUT, new_value.type(),
                        GetAnyFromStringFunctor<T>());
      entry = createEntryImpl(key, new_port);
    }
    lock.lock();

    entry->value = new_value;
    entry->sequence_id++;
    entry->stamp = std::chrono::steady_clock::now().time_since_epoch();
  }
  else
  {
    // this is not the first time we set this entry, we need to check
    // if the type is the same or not.
    Entry& entry = *it->second;
    std::scoped_lock scoped_lock(entry.entry_mutex);

    Any& previous_any = entry.value;
    Any new_value(value);

    // special case: entry exists but it is not strongly typed... yet
    if(!entry.info.isStronglyTyped())
    {
      // Use the new type to create a new entry that is strongly typed.
      entry.info = TypeInfo::Create<T>();
      entry.sequence_id++;
      entry.stamp = std::chrono::steady_clock::now().time_since_epoch();
      previous_any = std::move(new_value);
      return;
    }

    std::type_index previous_type = entry.info.type();

    // Allow mismatch if going from vector -> vector<Any>.
    auto prev_type_demangled = BT::demangle(entry.value.type());
    bool previous_is_vector = BT::isVector(prev_type_demangled);
    bool new_is_vector_any = new_value.type() == typeid(std::vector<Any>);

    // check type mismatch
    if(previous_type != std::type_index(typeid(T)) && previous_type != new_value.type() &&
       !(previous_is_vector && new_is_vector_any))
    {
      bool mismatching = true;
      if(std::is_constructible<StringView, T>::value)
      {
        Any any_from_string = entry.info.parseString(value);
        if(any_from_string.empty() == false)
        {
          mismatching = false;
          new_value = std::move(any_from_string);
        }
      }
      // check if we are doing a safe cast between numbers
      // for instance, it is safe to use int(100) to set
      // a uint8_t port, but not int(-42) or int(300)
      if constexpr(std::is_arithmetic_v<T>)
      {
        if(mismatching && isCastingSafe(previous_type, value))
        {
          mismatching = false;
        }
      }

      if(mismatching)
      {
        debugMessage();

        auto msg = StrCat("Blackboard::set(", key,
                          "): once declared, "
                          "the type of a port shall not change. "
                          "Previously declared type [",
                          BT::demangle(previous_type), "], current type [",
                          BT::demangle(typeid(T)), "]");
        throw LogicError(msg);
      }
    }
    // if doing set<BT::Any>, skip type check
    if constexpr(std::is_same_v<Any, T>)
    {
      previous_any = new_value;
    }
    else
    {
      // copy only if the type is compatible
      new_value.copyInto(previous_any);
    }
    entry.sequence_id++;
    entry.stamp = std::chrono::steady_clock::now().time_since_epoch();
  }
}

template <typename T>
inline bool Blackboard::get(const std::string& key, T& value) const
{
  if(auto any_ref = getAnyLocked(key))
  {
    if(any_ref.get()->empty())
    {
      return false;
    }
    value = any_ref.get()->cast<T>();
    return true;
  }
  return false;
}

template <typename T>
inline Expected<Timestamp> Blackboard::getStamped(const std::string& key, T& value) const
{
  if(auto entry = getEntry(key))
  {
    std::unique_lock lk(entry->entry_mutex);
    if(entry->value.empty())
    {
      return nonstd::make_unexpected(StrCat("Blackboard::getStamped() error. Entry [",
                                            key, "] hasn't been initialized, yet"));
    }
    value = entry->value.cast<T>();
    return Timestamp{ entry->sequence_id, entry->stamp };
  }
  return nonstd::make_unexpected(
      StrCat("Blackboard::getStamped() error. Missing key [", key, "]"));
}

template <typename T>
inline Expected<StampedValue<T>> Blackboard::getStamped(const std::string& key) const
{
  StampedValue<T> out;
  if(auto res = getStamped<T>(key, out.value))
  {
    out.stamp = *res;
    return out;
  }
  else
  {
    return nonstd::make_unexpected(res.error());
  }
}

}  // namespace BT
