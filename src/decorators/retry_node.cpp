/* Copyright (C) 2015-2018 Michele Colledanchise -  All Rights Reserved
 * Copyright (C) 2018-2020 Davide Faconti, Eurecat -  All Rights Reserved
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

#include "behaviortree_cpp/decorators/retry_node.h"

namespace BT
{
constexpr const char* RetryNode::NUM_ATTEMPTS;

RetryNode::RetryNode(const std::string& name, int NTries)
  : DecoratorNode(name, {})
  , max_attempts_(NTries)
  , try_count_(0)
  , read_parameter_from_ports_(false)
{
  setRegistrationID("RetryUntilSuccessful");
}

RetryNode::RetryNode(const std::string& name, const NodeConfig& config)
  : DecoratorNode(name, config)
  , max_attempts_(0)
  , try_count_(0)
  , read_parameter_from_ports_(true)
{}

void RetryNode::halt()
{
  try_count_ = 0;
  DecoratorNode::halt();
}

NodeStatus RetryNode::tick()
{
  if(read_parameter_from_ports_)
  {
    if(!getInput(NUM_ATTEMPTS, max_attempts_))
    {
      throw RuntimeError("Missing parameter [", NUM_ATTEMPTS, "] in RetryNode");
    }
  }

  bool do_loop = try_count_ < max_attempts_ || max_attempts_ == -1;
  setStatus(NodeStatus::RUNNING);

  while(do_loop)
  {
    NodeStatus prev_status = child_node_->status();
    NodeStatus child_status = child_node_->executeTick();

    switch(child_status)
    {
      case NodeStatus::SUCCESS: {
        try_count_ = 0;
        resetChild();
        return (NodeStatus::SUCCESS);
      }

      case NodeStatus::FAILURE: {
        try_count_++;
        // Refresh max_attempts_ in case it changed in one of the child nodes
        getInput(NUM_ATTEMPTS, max_attempts_);
        do_loop = try_count_ < max_attempts_ || max_attempts_ == -1;

        resetChild();

        // Return the execution flow if the child is async,
        // to make this interruptable.
        if(requiresWakeUp() && prev_status == NodeStatus::IDLE && do_loop)
        {
          emitWakeUpSignal();
          return NodeStatus::RUNNING;
        }
      }
      break;

      case NodeStatus::RUNNING: {
        return NodeStatus::RUNNING;
      }

      case NodeStatus::SKIPPED: {
        // to allow it to be skipped again, we must reset the node
        resetChild();
        // the child has been skipped. Slip this too
        return NodeStatus::SKIPPED;
      }

      case NodeStatus::IDLE: {
        throw LogicError("[", name(), "]: A children should not return IDLE");
      }
    }
  }

  try_count_ = 0;
  return NodeStatus::FAILURE;
}

}  // namespace BT
