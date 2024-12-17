/* Copyright (C) 2015-2018 Michele Colledanchise -  All Rights Reserved
 * Copyright (C) 2018-2022 Davide Faconti, Eurecat -  All Rights Reserved
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "behaviortree_cpp/tree_node.h"
#include <cstring>
#include <array>

namespace BT
{

struct TreeNode::PImpl
{
  PImpl(std::string name, NodeConfig config)
    : name(std::move(name)), config(std::move(config))
  {}

  const std::string name;

  NodeStatus status = NodeStatus::IDLE;

  std::condition_variable state_condition_variable;

  mutable std::mutex state_mutex;

  StatusChangeSignal state_change_signal;

  NodeConfig config;

  std::string registration_ID;

  PreTickCallback pre_tick_callback;
  PostTickCallback post_tick_callback;
  TickMonitorCallback tick_monitor_callback;

  std::mutex callback_injection_mutex;

  std::shared_ptr<WakeUpSignal> wake_up;

  std::array<ScriptFunction, size_t(PreCond::COUNT_)> pre_parsed;
  std::array<ScriptFunction, size_t(PostCond::COUNT_)> post_parsed;
};

TreeNode::TreeNode(std::string name, NodeConfig config)
  : _p(new PImpl(std::move(name), std::move(config)))
{}

TreeNode::TreeNode(TreeNode&& other) noexcept
{
  this->_p = std::move(other._p);
}

TreeNode& TreeNode::operator=(TreeNode&& other) noexcept
{
  this->_p = std::move(other._p);
  return *this;
}

TreeNode::~TreeNode()
{}

NodeStatus TreeNode::executeTick()
{
  auto new_status = _p->status;
  PreTickCallback pre_tick;
  PostTickCallback post_tick;
  TickMonitorCallback monitor_tick;
  {
    std::scoped_lock lk(_p->callback_injection_mutex);
    pre_tick = _p->pre_tick_callback;
    post_tick = _p->post_tick_callback;
    monitor_tick = _p->tick_monitor_callback;
  }

  // a pre-condition may return the new status.
  // In this case it override the actual tick()
  if(auto precond = checkPreConditions())
  {
    new_status = precond.value();
  }
  else
  {
    // injected pre-callback
    bool substituted = false;
    if(pre_tick && !isStatusCompleted(_p->status))
    {
      auto override_status = pre_tick(*this);
      if(isStatusCompleted(override_status))
      {
        // don't execute the actual tick()
        substituted = true;
        new_status = override_status;
      }
    }

    // Call the ACTUAL tick
    if(!substituted)
    {
      using namespace std::chrono;

      auto t1 = steady_clock::now();
      // trick to prevent the compile from reordering the order of execution. See #861
      // This makes sure that the code is executed at the end of this scope
      std::shared_ptr<void> execute_later(nullptr, [&](...) {
        auto t2 = steady_clock::now();
        if(monitor_tick)
        {
          monitor_tick(*this, new_status, duration_cast<microseconds>(t2 - t1));
        }
      });

      new_status = tick();
    }
  }

  // injected post callback
  if(isStatusCompleted(new_status))
  {
    checkPostConditions(new_status);
  }

  if(post_tick)
  {
    auto override_status = post_tick(*this, new_status);
    if(isStatusCompleted(override_status))
    {
      new_status = override_status;
    }
  }

  // preserve the IDLE state if skipped, but communicate SKIPPED to parent
  if(new_status != NodeStatus::SKIPPED)
  {
    setStatus(new_status);
  }
  return new_status;
}

void TreeNode::haltNode()
{
  halt();

  const auto& parse_executor = _p->post_parsed[size_t(PostCond::ON_HALTED)];
  if(parse_executor)
  {
    Ast::Environment env = { config().blackboard, config().enums };
    parse_executor(env);
  }
}

void TreeNode::setStatus(NodeStatus new_status)
{
  if(new_status == NodeStatus::IDLE)
  {
    throw RuntimeError("Node [", name(),
                       "]: you are not allowed to set manually the status to IDLE. "
                       "If you know what you are doing (?) use resetStatus() instead.");
  }

  NodeStatus prev_status;
  {
    std::unique_lock<std::mutex> UniqueLock(_p->state_mutex);
    prev_status = _p->status;
    _p->status = new_status;
  }
  if(prev_status != new_status)
  {
    _p->state_condition_variable.notify_all();
    _p->state_change_signal.notify(std::chrono::high_resolution_clock::now(), *this,
                                   prev_status, new_status);
  }
}

TreeNode::PreScripts& TreeNode::preConditionsScripts()
{
  return _p->pre_parsed;
}

TreeNode::PostScripts& TreeNode::postConditionsScripts()
{
  return _p->post_parsed;
}

Expected<NodeStatus> TreeNode::checkPreConditions()
{
  Ast::Environment env = { config().blackboard, config().enums };

  // check the pre-conditions
  for(size_t index = 0; index < size_t(PreCond::COUNT_); index++)
  {
    const auto& parse_executor = _p->pre_parsed[index];
    if(!parse_executor)
    {
      continue;
    }

    const PreCond preID = PreCond(index);

    // Some preconditions are applied only when the node state is IDLE or SKIPPED
    if(_p->status == NodeStatus::IDLE || _p->status == NodeStatus::SKIPPED)
    {
      auto execute_result = parse_executor(env);
      // always
      if(preID == PreCond::ALWAYS)
      {
        return nonstd::make_unexpected("");
      }
      auto bool_result = execute_result.cast<bool>();
      // what to do if the condition is true and the precondition is xx_IF
      if(bool_result && preID == PreCond::FAILURE_IF)
      {
        return NodeStatus::FAILURE;
      }
      if(bool_result && preID == PreCond::SUCCESS_IF)
      {
        return NodeStatus::SUCCESS;
      }
      if(bool_result && preID == PreCond::SKIP_IF)
      {
        return NodeStatus::SKIPPED;
      }
      // if the condition is false and the precondition is WHILE_TRUE, skip the node
      if(!bool_result && preID == PreCond::WHILE_TRUE)
      {
        return NodeStatus::SKIPPED;
      }
    }
    else if(_p->status == NodeStatus::RUNNING && preID == PreCond::WHILE_TRUE)
    {
      // what to do if the condition is false
      if(!parse_executor(env).cast<bool>())
      {
        haltNode();
        return NodeStatus::SKIPPED;
      }
    }
  }
  return nonstd::make_unexpected("");  // no precondition
}

void TreeNode::checkPostConditions(NodeStatus status)
{
  auto ExecuteScript = [this](const PostCond& cond) {
    const auto& parse_executor = _p->post_parsed[size_t(cond)];
    if(parse_executor)
    {
      Ast::Environment env = { config().blackboard, config().enums };
      parse_executor(env);
    }
  };

  if(status == NodeStatus::SUCCESS)
  {
    ExecuteScript(PostCond::ON_SUCCESS);
  }
  else if(status == NodeStatus::FAILURE)
  {
    ExecuteScript(PostCond::ON_FAILURE);
  }
  ExecuteScript(PostCond::ALWAYS);
}

void TreeNode::resetStatus()
{
  NodeStatus prev_status;
  {
    std::unique_lock<std::mutex> lock(_p->state_mutex);
    prev_status = _p->status;
    _p->status = NodeStatus::IDLE;
  }

  if(prev_status != NodeStatus::IDLE)
  {
    _p->state_condition_variable.notify_all();
    _p->state_change_signal.notify(std::chrono::high_resolution_clock::now(), *this,
                                   prev_status, NodeStatus::IDLE);
  }
}

NodeStatus TreeNode::status() const
{
  std::lock_guard<std::mutex> lock(_p->state_mutex);
  return _p->status;
}

NodeStatus TreeNode::waitValidStatus()
{
  std::unique_lock<std::mutex> lock(_p->state_mutex);

  while(isHalted())
  {
    _p->state_condition_variable.wait(lock);
  }
  return _p->status;
}

const std::string& TreeNode::name() const
{
  return _p->name;
}

bool TreeNode::isHalted() const
{
  return _p->status == NodeStatus::IDLE;
}

TreeNode::StatusChangeSubscriber
TreeNode::subscribeToStatusChange(TreeNode::StatusChangeCallback callback)
{
  return _p->state_change_signal.subscribe(std::move(callback));
}

void TreeNode::setPreTickFunction(PreTickCallback callback)
{
  std::unique_lock lk(_p->callback_injection_mutex);
  _p->pre_tick_callback = callback;
}

void TreeNode::setPostTickFunction(PostTickCallback callback)
{
  std::unique_lock lk(_p->callback_injection_mutex);
  _p->post_tick_callback = callback;
}

void TreeNode::setTickMonitorCallback(TickMonitorCallback callback)
{
  std::unique_lock lk(_p->callback_injection_mutex);
  _p->tick_monitor_callback = callback;
}

uint16_t TreeNode::UID() const
{
  return _p->config.uid;
}

const std::string& TreeNode::fullPath() const
{
  return _p->config.path;
}

const std::string& TreeNode::registrationName() const
{
  return _p->registration_ID;
}

const NodeConfig& TreeNode::config() const
{
  return _p->config;
}

NodeConfig& TreeNode::config()
{
  return _p->config;
}

StringView TreeNode::getRawPortValue(const std::string& key) const
{
  auto remap_it = _p->config.input_ports.find(key);
  if(remap_it == _p->config.input_ports.end())
  {
    remap_it = _p->config.output_ports.find(key);
    if(remap_it == _p->config.output_ports.end())
    {
      throw std::logic_error(StrCat("[", key, "] not found"));
    }
  }
  return remap_it->second;
}

bool TreeNode::isBlackboardPointer(StringView str, StringView* stripped_pointer)
{
  if(str.size() < 3)
  {
    return false;
  }
  // strip leading and following spaces
  size_t front_index = 0;
  size_t last_index = str.size() - 1;
  while(str[front_index] == ' ' && front_index <= last_index)
  {
    front_index++;
  }
  while(str[last_index] == ' ' && front_index <= last_index)
  {
    last_index--;
  }
  const auto size = (last_index - front_index) + 1;
  auto valid = size >= 3 && str[front_index] == '{' && str[last_index] == '}';
  if(valid && stripped_pointer)
  {
    *stripped_pointer = StringView(&str[front_index + 1], size - 2);
  }
  return valid;
}

StringView TreeNode::stripBlackboardPointer(StringView str)
{
  StringView out;
  if(isBlackboardPointer(str, &out))
  {
    return out;
  }
  return {};
}

Expected<StringView> TreeNode::getRemappedKey(StringView port_name,
                                              StringView remapped_port)
{
  if(remapped_port == "{=}" || remapped_port == "=")
  {
    return { port_name };
  }
  StringView stripped;
  if(isBlackboardPointer(remapped_port, &stripped))
  {
    return { stripped };
  }
  return nonstd::make_unexpected("Not a blackboard pointer");
}

void TreeNode::emitWakeUpSignal()
{
  if(_p->wake_up)
  {
    _p->wake_up->emitSignal();
  }
}

bool TreeNode::requiresWakeUp() const
{
  return bool(_p->wake_up);
}

void TreeNode::setRegistrationID(StringView ID)
{
  _p->registration_ID.assign(ID.data(), ID.size());
}

void TreeNode::setWakeUpInstance(std::shared_ptr<WakeUpSignal> instance)
{
  _p->wake_up = instance;
}

void TreeNode::modifyPortsRemapping(const PortsRemapping& new_remapping)
{
  for(const auto& new_it : new_remapping)
  {
    auto it = _p->config.input_ports.find(new_it.first);
    if(it != _p->config.input_ports.end())
    {
      it->second = new_it.second;
    }
    it = _p->config.output_ports.find(new_it.first);
    if(it != _p->config.output_ports.end())
    {
      it->second = new_it.second;
    }
  }
}

template <>
std::string toStr<PreCond>(const PreCond& pre)
{
  switch(pre)
  {
    case PreCond::SUCCESS_IF:
      return "_successIf";
    case PreCond::FAILURE_IF:
      return "_failureIf";
    case PreCond::SKIP_IF:
      return "_skipIf";
    case PreCond::WHILE_TRUE:
      return "_while";
    case PreCond::ALWAYS:
      return "_onStart";
    default:
      return "Undefined";
  }
}

template <>
std::string toStr<PostCond>(const PostCond& pre)
{
  switch(pre)
  {
    case PostCond::ON_SUCCESS:
      return "_onSuccess";
    case PostCond::ON_FAILURE:
      return "_onFailure";
    case PostCond::ALWAYS:
      return "_post";
    case PostCond::ON_HALTED:
      return "_onHalted";
    default:
      return "Undefined";
  }
}

AnyPtrLocked BT::TreeNode::getLockedPortContent(const std::string& key)
{
  if(auto remapped_key = getRemappedKey(key, getRawPortValue(key)))
  {
    return _p->config.blackboard->getAnyLocked(std::string(*remapped_key));
  }
  return {};
}

}  // namespace BT
