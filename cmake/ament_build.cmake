#---- Add the subdirectory cmake ----
set(CMAKE_CONFIG_PATH ${CMAKE_MODULE_PATH}  "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CONFIG_PATH}")

if(BTCPP_GROOT_INTERFACE)
    find_package(ZeroMQ REQUIRED)
endif()

if(BTCPP_SQLITE_LOGGING)
    find_package(SQLite3 REQUIRED)
endif()

find_package(ament_index_cpp REQUIRED

set(BTCPP_EXTRA_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIRS}
                             ${SQLite3_INCLUDE_DIRS})

set( BTCPP_EXTRA_LIBRARIES
    $<BUILD_INTERFACE:ament_index_cpp::ament_index_cpp>
    $<BUILD_INTERFACE:${ZeroMQ_LIBRARIES}>
    $<BUILD_INTERFACE:${SQLite3_LIBRARIES}>
)

ament_export_dependencies(ament_index_cpp)

set( BTCPP_LIB_DESTINATION     lib )
set( BTCPP_INCLUDE_DESTINATION include )
set( BTCPP_BIN_DESTINATION     bin )

mark_as_advanced(
    BTCPP_EXTRA_LIBRARIES
    BTCPP_EXTRA_INCLUDE_DIRS
    BTCPP_LIB_DESTINATION
    BTCPP_INCLUDE_DESTINATION
    BTCPP_BIN_DESTINATION )

macro(export_btcpp_package)
    ament_export_include_directories(include)
    ament_export_libraries(${BTCPP_LIBRARY})
    ament_export_targets(${BTCPP_LIBRARY}Targets)
    ament_package()
endmacro()
