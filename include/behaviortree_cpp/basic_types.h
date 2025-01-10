#pragma once

#include <chrono>
#include <iostream>
#include <functional>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "behaviortree_cpp/utils/safe_any.hpp"
#include "behaviortree_cpp/exceptions.h"
#include "behaviortree_cpp/contrib/expected.hpp"

namespace BT
{
/// Enumerates the possible types of nodes
enum class NodeType
{
  UNDEFINED = 0,
  ACTION,
  CONDITION,
  CONTROL,
  DECORATOR,
  SUBTREE
};

/// Enumerates the states every node can be in after execution during a particular
/// time step.
/// IMPORTANT: Your custom nodes should NEVER return IDLE.
enum class NodeStatus
{
  IDLE = 0,
  RUNNING = 1,
  SUCCESS = 2,
  FAILURE = 3,
  SKIPPED = 4,
};

inline bool isStatusActive(const NodeStatus& status)
{
  return status != NodeStatus::IDLE && status != NodeStatus::SKIPPED;
}

inline bool isStatusCompleted(const NodeStatus& status)
{
  return status == NodeStatus::SUCCESS || status == NodeStatus::FAILURE;
}

enum class PortDirection
{
  INPUT,
  OUTPUT,
  INOUT
};

using StringView = std::string_view;

bool StartWith(StringView str, StringView prefix);

bool StartWith(StringView str, char prefix);

// vector of key/value pairs
using KeyValueVector = std::vector<std::pair<std::string, std::string>>;

/** Usage: given a function/method like this:
 *
 *     Expected<double> getAnswer();
 *
 * User code can check result and error message like this:
 *
 *     auto res = getAnswer();
 *     if( res )
 *     {
 *         std::cout << "answer was: " << res.value() << std::endl;
 *     }
 *     else{
 *         std::cerr << "failed to get the answer: " << res.error() << std::endl;
 *     }
 *
 * */
template <typename T>
using Expected = nonstd::expected<T, std::string>;

struct AnyTypeAllowed
{
};

/**
 * @brief convertFromJSON will parse a json string and use JsonExporter
 * to convert its content to a given type. It will work only if
 * the type was previously registered. May throw if it fails.
 *
 * @param json_text a valid JSON string
 * @param type you must specify the typeid()
 * @return the object, wrapped in Any.
 */
[[nodiscard]] Any convertFromJSON(StringView json_text, std::type_index type);

/// Same as the non template version, but with automatic casting
template <typename T>
[[nodiscard]] inline T convertFromJSON(StringView str)
{
  return convertFromJSON(str, typeid(T)).cast<T>();
}

/**
 * convertFromString is used to convert a string into a custom type.
 *
 * This function is invoked under the hood by TreeNode::getInput(), but only when the
 * input port contains a string.
 *
 * If you have a custom type, you need to implement the corresponding
 * template specialization.
 *
 * If the string starts with the prefix "json:", it will
 * fall back to convertFromJSON()
 */
template <typename T>
[[nodiscard]] inline T convertFromString(StringView str)
{
  // if string starts with "json:{", try to parse it as json
  if(StartWith(str, "json:"))
  {
    str.remove_prefix(5);
    return convertFromJSON<T>(str);
  }

  auto type_name = BT::demangle(typeid(T));

  std::cerr << "You (maybe indirectly) called BT::convertFromString() for type ["
            << type_name << "], but I can't find the template specialization.\n"
            << std::endl;

  throw LogicError(std::string("You didn't implement the template specialization of "
                               "convertFromString for this type: ") +
                   type_name);
}

template <>
[[nodiscard]] std::string convertFromString<std::string>(StringView str);

template <>
[[nodiscard]] const char* convertFromString<const char*>(StringView str);

template <>
[[nodiscard]] int8_t convertFromString<int8_t>(StringView str);

template <>
[[nodiscard]] int16_t convertFromString<int16_t>(StringView str);

template <>
[[nodiscard]] int32_t convertFromString<int32_t>(StringView str);

template <>
[[nodiscard]] int64_t convertFromString<int64_t>(StringView str);

template <>
[[nodiscard]] uint8_t convertFromString<uint8_t>(StringView str);

template <>
[[nodiscard]] uint16_t convertFromString<uint16_t>(StringView str);

template <>
[[nodiscard]] uint32_t convertFromString<uint32_t>(StringView str);

template <>
[[nodiscard]] uint64_t convertFromString<uint64_t>(StringView str);

template <>
[[nodiscard]] float convertFromString<float>(StringView str);

template <>
[[nodiscard]] double convertFromString<double>(StringView str);

// Integer numbers separated by the character ";"
template <>
[[nodiscard]] std::vector<int> convertFromString<std::vector<int>>(StringView str);

// Real numbers separated by the character ";"
template <>
[[nodiscard]] std::vector<double> convertFromString<std::vector<double>>(StringView str);

// Strings separated by the character ";"
template <>
[[nodiscard]] std::vector<std::string>
convertFromString<std::vector<std::string>>(StringView str);

// This recognizes either 0/1, true/false, TRUE/FALSE
template <>
[[nodiscard]] bool convertFromString<bool>(StringView str);

// Names with all capital letters
template <>
[[nodiscard]] NodeStatus convertFromString<NodeStatus>(StringView str);

// Names with all capital letters
template <>
[[nodiscard]] NodeType convertFromString<NodeType>(StringView str);

template <>
[[nodiscard]] PortDirection convertFromString<PortDirection>(StringView str);

using StringConverter = std::function<Any(StringView)>;

using StringConvertersMap = std::unordered_map<const std::type_info*, StringConverter>;

// helper function
template <typename T>
[[nodiscard]] inline StringConverter GetAnyFromStringFunctor()
{
  if constexpr(std::is_constructible_v<StringView, T>)
  {
    return [](StringView str) { return Any(str); };
  }
  else if constexpr(std::is_same_v<BT::AnyTypeAllowed, T> || std::is_enum_v<T>)
  {
    return {};
  }
  else
  {
    return [](StringView str) { return Any(convertFromString<T>(str)); };
  }
}

template <>
[[nodiscard]] inline StringConverter GetAnyFromStringFunctor<void>()
{
  return {};
}

//------------------------------------------------------------------

template <typename T>
constexpr bool IsConvertibleToString()
{
  return std::is_convertible_v<T, std::string> ||
         std::is_convertible_v<T, std::string_view>;
}

Expected<std::string> toJsonString(const Any& value);

/**
 * @brief toStr is the reverse operation of convertFromString.
 *
 * If T is a custom type and there is no template specialization,
 * it will try to fall back to toJsonString()
 */
template <typename T>
[[nodiscard]] std::string toStr(const T& value)
{
  if constexpr(IsConvertibleToString<T>())
  {
    return static_cast<std::string>(value);
  }
  else if constexpr(!std::is_arithmetic_v<T>)
  {
    if(auto str = toJsonString(Any(value)))
    {
      return *str;
    }

    throw LogicError(StrCat("Function BT::toStr<T>() not specialized for type [",
                            BT::demangle(typeid(T)), "]"));
  }
  else
  {
    return std::to_string(value);
  }
}

template <>
[[nodiscard]] std::string toStr<bool>(const bool& value);

template <>
[[nodiscard]] std::string toStr<std::string>(const std::string& value);

template <>
[[nodiscard]] std::string toStr<BT::NodeStatus>(const BT::NodeStatus& status);

/**
 * @brief toStr converts NodeStatus to string. Optionally colored.
 */
[[nodiscard]] std::string toStr(BT::NodeStatus status, bool colored);

std::ostream& operator<<(std::ostream& os, const BT::NodeStatus& status);

template <>
[[nodiscard]] std::string toStr<BT::NodeType>(const BT::NodeType& type);

std::ostream& operator<<(std::ostream& os, const BT::NodeType& type);

template <>
[[nodiscard]] std::string toStr<BT::PortDirection>(const BT::PortDirection& direction);

std::ostream& operator<<(std::ostream& os, const BT::PortDirection& type);

// Small utility, unless you want to use <boost/algorithm/string.hpp>
[[nodiscard]] std::vector<StringView> splitString(const StringView& strToSplit,
                                                  char delimeter);

template <typename Predicate>
using enable_if = typename std::enable_if<Predicate::value>::type*;

template <typename Predicate>
using enable_if_not = typename std::enable_if<!Predicate::value>::type*;

#ifdef USE_BTCPP3_OLD_NAMES
// note: we also use the name Optional instead of expected because it is more intuitive
// for users that are not up to date with "modern" C++
template <typename T>
using Optional = nonstd::expected<T, std::string>;
#endif

/** Usage: given a function/method like:
 *
 *     Result DoSomething();
 *
 * User code can check result and error message like this:
 *
 *     auto res = DoSomething();
 *     if( res )
 *     {
 *         std::cout << "DoSomething() done " << std::endl;
 *     }
 *     else{
 *         std::cerr << "DoSomething() failed with message: " << res.error() << std::endl;
 *     }
 *
 * */
using Result = Expected<std::monostate>;

struct Timestamp
{
  // Number being incremented every time a new value is written
  uint64_t seq = 0;
  // Last update time. Nanoseconds since epoch
  std::chrono::nanoseconds time = std::chrono::nanoseconds(0);
};

[[nodiscard]] bool IsAllowedPortName(StringView str);

[[nodiscard]] bool IsReservedAttribute(StringView str);

class TypeInfo
{
public:
  template <typename T>
  static TypeInfo Create()
  {
    return TypeInfo{ typeid(T), GetAnyFromStringFunctor<T>() };
  }

  TypeInfo() : type_info_(typeid(AnyTypeAllowed)), type_str_("AnyTypeAllowed")
  {}

  TypeInfo(std::type_index type_info, StringConverter conv)
    : type_info_(type_info), converter_(conv), type_str_(BT::demangle(type_info))
  {}

  [[nodiscard]] const std::type_index& type() const;

  [[nodiscard]] const std::string& typeName() const;

  [[nodiscard]] Any parseString(const char* str) const;

  [[nodiscard]] Any parseString(const std::string& str) const;

  template <typename T>
  [[nodiscard]] Any parseString(const T&) const
  {
    // avoid compilation errors
    return {};
  }

  [[nodiscard]] bool isStronglyTyped() const
  {
    return type_info_ != typeid(AnyTypeAllowed) && type_info_ != typeid(BT::Any);
  }

  [[nodiscard]] const StringConverter& converter() const
  {
    return converter_;
  }

private:
  std::type_index type_info_;
  StringConverter converter_;
  std::string type_str_;
};

class PortInfo : public TypeInfo
{
public:
  PortInfo(PortDirection direction = PortDirection::INOUT)
    : TypeInfo(), direction_(direction)
  {}

  PortInfo(PortDirection direction, std::type_index type_info, StringConverter conv)
    : TypeInfo(type_info, conv), direction_(direction)
  {}

  [[nodiscard]] PortDirection direction() const;

  void setDescription(StringView description);

  template <typename T>
  void setDefaultValue(const T& default_value)
  {
    default_value_ = Any(default_value);
    try
    {
      default_value_str_ = BT::toStr(default_value);
    }
    catch(LogicError&)
    {}
  }

  [[nodiscard]] const std::string& description() const;

  [[nodiscard]] const Any& defaultValue() const;

  [[nodiscard]] const std::string& defaultValueString() const;

private:
  PortDirection direction_;
  std::string description_;
  Any default_value_;
  std::string default_value_str_;
};

template <typename T = AnyTypeAllowed>
[[nodiscard]] std::pair<std::string, PortInfo> CreatePort(PortDirection direction,
                                                          StringView name,
                                                          StringView description = {})
{
  auto sname = static_cast<std::string>(name);
  if(!IsAllowedPortName(sname))
  {
    throw RuntimeError("The name of a port must not be `name` or `ID` "
                       "and must start with an alphabetic character. "
                       "Underscore is reserved.");
  }

  std::pair<std::string, PortInfo> out;

  if(std::is_same<T, void>::value)
  {
    out = { sname, PortInfo(direction) };
  }
  else
  {
    out = { sname, PortInfo(direction, typeid(T), GetAnyFromStringFunctor<T>()) };
  }
  if(!description.empty())
  {
    out.second.setDescription(description);
  }
  return out;
}

//----------
/** Syntactic sugar to invoke CreatePort<T>(PortDirection::INPUT, ...)
 *
 *  @param name the name of the port
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed>
[[nodiscard]] inline std::pair<std::string, PortInfo>
InputPort(StringView name, StringView description = {})
{
  return CreatePort<T>(PortDirection::INPUT, name, description);
}

/** Syntactic sugar to invoke CreatePort<T>(PortDirection::OUTPUT,...)
 *
 *  @param name the name of the port
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed>
[[nodiscard]] inline std::pair<std::string, PortInfo>
OutputPort(StringView name, StringView description = {})
{
  return CreatePort<T>(PortDirection::OUTPUT, name, description);
}

/** Syntactic sugar to invoke CreatePort<T>(PortDirection::INOUT,...)
 *
 *  @param name the name of the port
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed>
[[nodiscard]] inline std::pair<std::string, PortInfo>
BidirectionalPort(StringView name, StringView description = {})
{
  return CreatePort<T>(PortDirection::INOUT, name, description);
}
//----------

namespace details
{

template <typename T = AnyTypeAllowed, typename DefaultT = T>
[[nodiscard]] inline std::pair<std::string, PortInfo>
PortWithDefault(PortDirection direction, StringView name, const DefaultT& default_value,
                StringView description)
{
  static_assert(IsConvertibleToString<DefaultT>() || std::is_convertible_v<T, DefaultT> ||
                    std::is_constructible_v<T, DefaultT>,
                "The default value must be either the same of the port or string");

  auto out = CreatePort<T>(direction, name, description);

  if constexpr(std::is_constructible_v<T, DefaultT>)
  {
    out.second.setDefaultValue(T(default_value));
  }
  else if constexpr(IsConvertibleToString<DefaultT>())
  {
    out.second.setDefaultValue(std::string(default_value));
  }
  else
  {
    out.second.setDefaultValue(default_value);
  }
  return out;
}

}  // end namespace details

/** Syntactic sugar to invoke CreatePort<T>(PortDirection::INPUT,...)
 *  It also sets the PortInfo::defaultValue()
 *
 *  @param name the name of the port
 *  @param default_value default value of the port, either type T of BlackboardKey
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed, typename DefaultT = T>
[[nodiscard]] inline std::pair<std::string, PortInfo>
InputPort(StringView name, const DefaultT& default_value, StringView description)
{
  return details::PortWithDefault<T, DefaultT>(PortDirection::INPUT, name, default_value,
                                               description);
}

/** Syntactic sugar to invoke CreatePort<T>(PortDirection::INOUT,...)
 *  It also sets the PortInfo::defaultValue()
 *
 *  @param name the name of the port
 *  @param default_value default value of the port, either type T of BlackboardKey
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed, typename DefaultT = T>
[[nodiscard]] inline std::pair<std::string, PortInfo>
BidirectionalPort(StringView name, const DefaultT& default_value, StringView description)
{
  return details::PortWithDefault<T, DefaultT>(PortDirection::INOUT, name, default_value,
                                               description);
}

/** Syntactic sugar to invoke CreatePort<T>(PortDirection::OUTPUT,...)
 *  It also sets the PortInfo::defaultValue()
 *
 *  @param name the name of the port
 *  @param default_value default blackboard entry where the output is written
 *  @param description optional human-readable description
 */
template <typename T = AnyTypeAllowed>
[[nodiscard]] inline std::pair<std::string, PortInfo> OutputPort(StringView name,
                                                                 StringView default_value,
                                                                 StringView description)
{
  if(default_value.empty() || default_value.front() != '{' || default_value.back() != '}')
  {
    throw LogicError("Output port can only refer to blackboard entries, i.e. use the "
                     "syntax '{port_name}'");
  }
  auto out = CreatePort<T>(PortDirection::OUTPUT, name, description);
  out.second.setDefaultValue(default_value);
  return out;
}

//----------

using PortsList = std::unordered_map<std::string, PortInfo>;

template <typename T, typename = void>
struct has_static_method_providedPorts : std::false_type
{
};

template <typename T>
struct has_static_method_providedPorts<
    T, typename std::enable_if<
           std::is_same<decltype(T::providedPorts()), PortsList>::value>::type>
  : std::true_type
{
};

template <typename T, typename = void>
struct has_static_method_metadata : std::false_type
{
};

template <typename T>
struct has_static_method_metadata<
    T, typename std::enable_if<
           std::is_same<decltype(T::metadata()), KeyValueVector>::value>::type>
  : std::true_type
{
};

template <typename T>
[[nodiscard]] inline PortsList
getProvidedPorts(enable_if<has_static_method_providedPorts<T>> = nullptr)
{
  return T::providedPorts();
}

template <typename T>
[[nodiscard]] inline PortsList
getProvidedPorts(enable_if_not<has_static_method_providedPorts<T>> = nullptr)
{
  return {};
}

using TimePoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::high_resolution_clock::duration;

}  // namespace BT
