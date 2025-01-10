#include "behaviortree_cpp/basic_types.h"
#include "behaviortree_cpp/tree_node.h"
#include "behaviortree_cpp/json_export.h"

#include <cstdlib>
#include <cstring>
#include <clocale>
#include <charconv>

namespace BT
{
template <>
std::string toStr<NodeStatus>(const NodeStatus& status)
{
  switch(status)
  {
    case NodeStatus::SUCCESS:
      return "SUCCESS";
    case NodeStatus::FAILURE:
      return "FAILURE";
    case NodeStatus::RUNNING:
      return "RUNNING";
    case NodeStatus::IDLE:
      return "IDLE";
    case NodeStatus::SKIPPED:
      return "SKIPPED";
  }
  return "";
}

template <>
[[nodiscard]] std::string toStr<bool>(const bool& value)
{
  return value ? "true" : "false";
}

template <>
std::string toStr<std::string>(const std::string& value)
{
  return value;
}

std::string toStr(NodeStatus status, bool colored)
{
  if(!colored)
  {
    return toStr(status);
  }
  else
  {
    switch(status)
    {
      case NodeStatus::SUCCESS:
        return "\x1b[32m"
               "SUCCESS"
               "\x1b[0m";  // RED
      case NodeStatus::FAILURE:
        return "\x1b[31m"
               "FAILURE"
               "\x1b[0m";  // GREEN
      case NodeStatus::RUNNING:
        return "\x1b[33m"
               "RUNNING"
               "\x1b[0m";  // YELLOW
      case NodeStatus::SKIPPED:
        return "\x1b[34m"
               "SKIPPED"
               "\x1b[0m";  // BLUE
      case NodeStatus::IDLE:
        return "\x1b[36m"
               "IDLE"
               "\x1b[0m";  // CYAN
    }
  }
  return "Undefined";
}

template <>
std::string toStr<PortDirection>(const PortDirection& direction)
{
  switch(direction)
  {
    case PortDirection::INPUT:
      return "Input";
    case PortDirection::OUTPUT:
      return "Output";
    case PortDirection::INOUT:
      return "InOut";
  }
  return "InOut";
}

template <>
std::string toStr<NodeType>(const NodeType& type)
{
  switch(type)
  {
    case NodeType::ACTION:
      return "Action";
    case NodeType::CONDITION:
      return "Condition";
    case NodeType::DECORATOR:
      return "Decorator";
    case NodeType::CONTROL:
      return "Control";
    case NodeType::SUBTREE:
      return "SubTree";
    default:
      return "Undefined";
  }
}

template <>
std::string convertFromString<std::string>(StringView str)
{
  return std::string(str.data(), str.size());
}

template <>
int64_t convertFromString<int64_t>(StringView str)
{
  long result = 0;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  if(ec != std::errc())
  {
    throw RuntimeError(StrCat("Can't convert string [", str, "] to integer"));
  }
  return result;
}

template <>
uint64_t convertFromString<uint64_t>(StringView str)
{
  unsigned long result = 0;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  if(ec != std::errc())
  {
    throw RuntimeError(StrCat("Can't convert string [", str, "] to integer"));
  }
  return result;
}

template <typename T>
T ConvertWithBoundCheck(StringView str)
{
  auto res = convertFromString<int64_t>(str);
  if(res < std::numeric_limits<T>::lowest() || res > std::numeric_limits<T>::max())
  {
    throw RuntimeError(
        StrCat("Value out of bound when converting [", str, "] to integer"));
  }
  return res;
}

template <>
int8_t convertFromString<int8_t>(StringView str)
{
  return ConvertWithBoundCheck<int8_t>(str);
}

template <>
int16_t convertFromString<int16_t>(StringView str)
{
  return ConvertWithBoundCheck<int16_t>(str);
}

template <>
int32_t convertFromString<int32_t>(StringView str)
{
  return ConvertWithBoundCheck<int32_t>(str);
}

template <>
uint8_t convertFromString<uint8_t>(StringView str)
{
  return ConvertWithBoundCheck<uint8_t>(str);
}

template <>
uint16_t convertFromString<uint16_t>(StringView str)
{
  return ConvertWithBoundCheck<uint16_t>(str);
}

template <>
uint32_t convertFromString<uint32_t>(StringView str)
{
  return ConvertWithBoundCheck<uint32_t>(str);
}

template <>
double convertFromString<double>(StringView str)
{
  // see issue #120
  // http://quick-bench.com/DWaXRWnxtxvwIMvZy2DxVPEKJnE

  std::string old_locale = setlocale(LC_NUMERIC, nullptr);
  setlocale(LC_NUMERIC, "C");
  double val = std::stod(str.data());
  setlocale(LC_NUMERIC, old_locale.c_str());
  return val;
}

template <>
float convertFromString<float>(StringView str)
{
  std::string old_locale = setlocale(LC_NUMERIC, nullptr);
  setlocale(LC_NUMERIC, "C");
  float val = std::stof(str.data());
  setlocale(LC_NUMERIC, old_locale.c_str());
  return val;
}

template <>
std::vector<int> convertFromString<std::vector<int>>(StringView str)
{
  auto parts = splitString(str, ';');
  std::vector<int> output;
  output.reserve(parts.size());
  for(const StringView& part : parts)
  {
    output.push_back(convertFromString<int>(part));
  }
  return output;
}

template <>
std::vector<double> convertFromString<std::vector<double>>(StringView str)
{
  auto parts = splitString(str, ';');
  std::vector<double> output;
  output.reserve(parts.size());
  for(const StringView& part : parts)
  {
    output.push_back(convertFromString<double>(part));
  }
  return output;
}

template <>
std::vector<std::string> convertFromString<std::vector<std::string>>(StringView str)
{
  auto parts = splitString(str, ';');
  std::vector<std::string> output;
  output.reserve(parts.size());
  for(const StringView& part : parts)
  {
    output.push_back(convertFromString<std::string>(part));
  }
  return output;
}

template <>
bool convertFromString<bool>(StringView str)
{
  if(str.size() == 1)
  {
    if(str[0] == '0')
    {
      return false;
    }
    if(str[0] == '1')
    {
      return true;
    }
  }
  else if(str.size() == 4)
  {
    if(str == "true" || str == "TRUE" || str == "True")
    {
      return true;
    }
  }
  else if(str.size() == 5)
  {
    if(str == "false" || str == "FALSE" || str == "False")
    {
      return false;
    }
  }
  throw RuntimeError("convertFromString(): invalid bool conversion");
}

template <>
NodeStatus convertFromString<NodeStatus>(StringView str)
{
  if(str == "IDLE")
    return NodeStatus::IDLE;
  if(str == "RUNNING")
    return NodeStatus::RUNNING;
  if(str == "SUCCESS")
    return NodeStatus::SUCCESS;
  if(str == "FAILURE")
    return NodeStatus::FAILURE;
  if(str == "SKIPPED")
    return NodeStatus::SKIPPED;

  throw RuntimeError(std::string("Cannot convert this to NodeStatus: ") +
                     static_cast<std::string>(str));
}

template <>
NodeType convertFromString<NodeType>(StringView str)
{
  if(str == "Action")
    return NodeType::ACTION;
  if(str == "Condition")
    return NodeType::CONDITION;
  if(str == "Control")
    return NodeType::CONTROL;
  if(str == "Decorator")
    return NodeType::DECORATOR;
  if(str == "SubTree")
    return NodeType::SUBTREE;
  return NodeType::UNDEFINED;
}

template <>
PortDirection convertFromString<PortDirection>(StringView str)
{
  if(str == "Input" || str == "INPUT")
    return PortDirection::INPUT;
  if(str == "Output" || str == "OUTPUT")
    return PortDirection::OUTPUT;
  if(str == "InOut" || str == "INOUT")
    return PortDirection::INOUT;
  throw RuntimeError(std::string("Cannot convert this to PortDirection: ") +
                     static_cast<std::string>(str));
}

std::ostream& operator<<(std::ostream& os, const NodeType& type)
{
  os << toStr(type);
  return os;
}

std::ostream& operator<<(std::ostream& os, const NodeStatus& status)
{
  os << toStr(status);
  return os;
}

std::ostream& operator<<(std::ostream& os, const PortDirection& type)
{
  os << toStr(type);
  return os;
}

std::vector<StringView> splitString(const StringView& strToSplit, char delimeter)
{
  std::vector<StringView> splitted_strings;
  splitted_strings.reserve(4);

  size_t pos = 0;
  while(pos < strToSplit.size())
  {
    size_t new_pos = strToSplit.find_first_of(delimeter, pos);
    if(new_pos == std::string::npos)
    {
      new_pos = strToSplit.size();
    }
    const auto sv = StringView{ &strToSplit.data()[pos], new_pos - pos };
    splitted_strings.push_back(sv);
    pos = new_pos + 1;
  }
  return splitted_strings;
}

PortDirection PortInfo::direction() const
{
  return direction_;
}

const std::type_index& TypeInfo::type() const
{
  return type_info_;
}

const std::string& TypeInfo::typeName() const
{
  return type_str_;
}

Any TypeInfo::parseString(const char* str) const
{
  if(converter_)
  {
    return converter_(str);
  }
  return {};
}

Any TypeInfo::parseString(const std::string& str) const
{
  if(converter_)
  {
    return converter_(str);
  }
  return {};
}

void PortInfo::setDescription(StringView description)
{
  description_ = static_cast<std::string>(description);
}

const std::string& PortInfo::description() const
{
  return description_;
}

const Any& PortInfo::defaultValue() const
{
  return default_value_;
}

const std::string& PortInfo::defaultValueString() const
{
  return default_value_str_;
}

bool IsAllowedPortName(StringView str)
{
  if(str.empty())
  {
    return false;
  }
  const char first_char = str.data()[0];
  if(!std::isalpha(first_char))
  {
    return false;
  }
  return !IsReservedAttribute(str);
}

bool IsReservedAttribute(StringView str)
{
  for(const auto& name : PreCondNames)
  {
    if(name == str)
    {
      return true;
    }
  }
  for(const auto& name : PostCondNames)
  {
    if(name == str)
    {
      return true;
    }
  }
  return str == "name" || str == "ID" || str == "_autoremap";
}

Any convertFromJSON(StringView json_text, std::type_index type)
{
  nlohmann::json json = nlohmann::json::parse(json_text);
  auto res = JsonExporter::get().fromJson(json, type);
  if(!res)
  {
    throw std::runtime_error(res.error());
  }
  return res->first;
}

Expected<std::string> toJsonString(const Any& value)
{
  nlohmann::json json;
  if(JsonExporter::get().toJson(value, json))
  {
    return StrCat("json:", json.dump());
  }
  return nonstd::make_unexpected("toJsonString failed");
}

bool StartWith(StringView str, StringView prefix)
{
  return str.size() >= prefix.size() &&
         strncmp(str.data(), prefix.data(), prefix.size()) == 0;
}

bool StartWith(StringView str, char prefix)
{
  return str.size() >= 1 && str[0] == prefix;
}

}  // namespace BT
