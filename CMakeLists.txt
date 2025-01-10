cmake_minimum_required(VERSION 3.16.3) # version on Ubuntu Focal

project(behaviortree_cpp VERSION 4.6.2 LANGUAGES C CXX)

set(CMAKE_CONFIG_PATH ${CMAKE_MODULE_PATH}  "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CONFIG_PATH}")

set(BTCPP_LIBRARY ${PROJECT_NAME})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN)
else()
    add_definitions(-Wpedantic -fno-omit-frame-pointer)
endif()


#---- project configuration ----
option(BTCPP_SHARED_LIBS "Build shared libraries" ON)
option(BTCPP_BUILD_TOOLS "Build commandline tools" ON)
option(BTCPP_EXAMPLES   "Build tutorials and examples" ON)
option(BTCPP_UNIT_TESTS "Build the unit tests" ON)
option(BTCPP_GROOT_INTERFACE "Add Groot2 connection. Requires ZeroMQ" ON)
option(BTCPP_SQLITE_LOGGING "Add SQLite logging." ON)

option(USE_V3_COMPATIBLE_NAMES  "Use some alias to compile more easily old 3.x code" OFF)

if(USE_V3_COMPATIBLE_NAMES)
    add_definitions(-DUSE_BTCPP3_OLD_NAMES)
endif()

#---- Find other packages ----
find_package(Threads REQUIRED)


set(BEHAVIOR_TREE_LIBRARY ${PROJECT_NAME})

# Update the policy setting to avoid an error when loading the ament_cmake package
# at the current cmake version level
if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()

find_package(ament_cmake QUIET)

if ( ament_cmake_FOUND )

    add_definitions( -DUSING_ROS2 )
    message(STATUS "------------------------------------------")
    message(STATUS "BehaviourTree is being built using AMENT.")
    message(STATUS "------------------------------------------")
    include(cmake/ament_build.cmake)

elseif( CATKIN_DEVEL_PREFIX OR CATKIN_BUILD_BINARY_PACKAGE)

    add_definitions( -DUSING_ROS )
    message(STATUS "------------------------------------------")
    message(STATUS "BehaviourTree is being built using CATKIN.")
    message(STATUS "------------------------------------------")
    include(cmake/catkin_build.cmake)
    set(catkin_FOUND TRUE)
else()
    message(STATUS "------------------------------------------")
    message(STATUS "BehaviourTree is being built with conan.")
    message(STATUS "------------------------------------------")
    include(cmake/conan_build.cmake)
endif()

#############################################################
# LIBRARY

add_subdirectory(3rdparty/lexy)

add_library(minitrace STATIC 3rdparty/minitrace/minitrace.cpp)
target_compile_definitions(minitrace PRIVATE MTR_ENABLED=True)
set_property(TARGET minitrace PROPERTY POSITION_INDEPENDENT_CODE ON)

list(APPEND BT_SOURCE
    src/action_node.cpp
    src/basic_types.cpp
    src/behavior_tree.cpp
    src/blackboard.cpp
    src/bt_factory.cpp
    src/decorator_node.cpp
    src/condition_node.cpp
    src/control_node.cpp
    src/shared_library.cpp
    src/tree_node.cpp
    src/script_parser.cpp
    src/json_export.cpp
    src/xml_parsing.cpp

    src/actions/test_node.cpp
    src/actions/sleep_node.cpp
    src/actions/updated_action.cpp

    src/decorators/delay_node.cpp
    src/decorators/inverter_node.cpp
    src/decorators/repeat_node.cpp
    src/decorators/retry_node.cpp
    src/decorators/subtree_node.cpp
    src/decorators/timeout_node.cpp
    src/decorators/updated_decorator.cpp

    src/controls/if_then_else_node.cpp
    src/controls/fallback_node.cpp
    src/controls/parallel_node.cpp
    src/controls/reactive_parallel_node.cpp
    src/controls/parallel_all_node.cpp
    src/controls/reactive_sequence.cpp
    src/controls/reactive_fallback.cpp
    src/controls/sequence_node.cpp
    src/controls/sequence_with_memory_node.cpp
    src/controls/switch_node.cpp
    src/controls/while_do_else_node.cpp

    src/loggers/bt_cout_logger.cpp
    src/loggers/bt_file_logger_v2.cpp
    src/loggers/bt_minitrace_logger.cpp
    src/loggers/bt_observer.cpp

    3rdparty/tinyxml2/tinyxml2.cpp
    )


if(BTCPP_GROOT_INTERFACE)
    # should be found already, at this stage
    list(APPEND BT_SOURCE  src/loggers/groot2_publisher.cpp )
endif()

if(BTCPP_SQLITE_LOGGING)
    list(APPEND BT_SOURCE  src/loggers/bt_sqlite_logger.cpp )
endif()

######################################################

if (UNIX)
    list(APPEND BT_SOURCE src/shared_library_UNIX.cpp )
endif()

if (WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
    list(APPEND BT_SOURCE src/shared_library_WIN.cpp )
endif()

if (BTCPP_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_library(${BTCPP_LIBRARY} SHARED ${BT_SOURCE})
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(${BTCPP_LIBRARY} STATIC ${BT_SOURCE})
endif()

message(STATUS "BTCPP_EXTRA_LIBRARIES: ${BTCPP_EXTRA_LIBRARIES}")

target_link_libraries(${BTCPP_LIBRARY}
    PRIVATE
        Threads::Threads
        ${CMAKE_DL_LIBS}
        $<BUILD_INTERFACE:foonathan::lexy>
        $<BUILD_INTERFACE:minitrace>
    PUBLIC
        ${BTCPP_EXTRA_LIBRARIES}
)

target_include_directories(${BTCPP_LIBRARY}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lexy/include>
        ${BTCPP_EXTRA_INCLUDE_DIRS}
    )

target_compile_definitions(${BTCPP_LIBRARY} PRIVATE $<$<CONFIG:Debug>:TINYXML2_DEBUG>)
target_compile_definitions(${BTCPP_LIBRARY} PUBLIC BTCPP_LIBRARY_VERSION="${CMAKE_PROJECT_VERSION}")

target_compile_features(${BTCPP_LIBRARY} PUBLIC cxx_std_17)

if(MSVC)
	target_compile_options(${BTCPP_LIBRARY} PRIVATE "/source-charset:utf-8")
else()
    target_compile_options(${BTCPP_LIBRARY} PRIVATE -Wall -Wextra)
endif()

add_library(BT::${BTCPP_LIBRARY} ALIAS ${BTCPP_LIBRARY})

#############################################################
message( STATUS "BTCPP_LIB_DESTINATION:   ${BTCPP_LIB_DESTINATION} " )
message( STATUS "BTCPP_INCLUDE_DESTINATION: ${BTCPP_INCLUDE_DESTINATION} " )
message( STATUS "BTCPP_UNIT_TESTS:   ${BTCPP_UNIT_TESTS} " )

if (BTCPP_UNIT_TESTS OR BTCPP_EXAMPLES)
add_subdirectory(sample_nodes)
endif()

######################################################

if (BTCPP_UNIT_TESTS)
    add_subdirectory(tests)
endif()

if(BTCPP_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BTCPP_EXAMPLES)
    add_subdirectory(examples)
endif()

######################################################
# INSTALL

INSTALL(TARGETS ${BTCPP_LIBRARY}
    EXPORT ${BTCPP_LIBRARY}Targets
    ARCHIVE DESTINATION ${BTCPP_LIB_DESTINATION}
    LIBRARY DESTINATION ${BTCPP_LIB_DESTINATION}
    RUNTIME DESTINATION ${BTCPP_BIN_DESTINATION}
    INCLUDES DESTINATION ${BTCPP_INCLUDE_DESTINATION}
    )

INSTALL( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${BTCPP_INCLUDE_DESTINATION}
    FILES_MATCHING PATTERN "*.h*")

export_btcpp_package()
