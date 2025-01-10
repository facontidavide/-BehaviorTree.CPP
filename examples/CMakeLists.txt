# include_directories( ../sample_nodes )

# set(CMAKE_DEBUG_POSTFIX "")

# function(CompileExample name)
#     add_executable(${name}  ${name}.cpp )
#     target_link_libraries(${name} ${BTCPP_LIBRARY}  bt_sample_nodes )
# endfunction()


# # The plugin libdummy_nodes.so can be linked statically or
# # loaded dynamically at run-time.
# add_executable(t01_first_tree_static  t01_build_your_first_tree.cpp )
# target_compile_definitions(t01_first_tree_static PRIVATE "MANUAL_STATIC_LINKING")
# target_link_libraries(t01_first_tree_static ${BTCPP_LIBRARY}   bt_sample_nodes )

# add_executable(t01_first_tree_dynamic  t01_build_your_first_tree.cpp )
# target_link_libraries(t01_first_tree_dynamic ${BTCPP_LIBRARY}  )

# CompileExample("t02_basic_ports")
# CompileExample("t03_generic_ports")
# CompileExample("t04_reactive_sequence")
# CompileExample("t05_crossdoor")
# CompileExample("t06_subtree_port_remapping")
# CompileExample("t07_load_multiple_xml")
# CompileExample("t08_additional_node_args")
# CompileExample("t09_scripting")
# CompileExample("t10_observer")

# if(BTCPP_GROOT_INTERFACE)
#   CompileExample("t11_groot_howto")
# endif()

# CompileExample("t12_default_ports")
# CompileExample("t13_access_by_ref")
# CompileExample("t14_subtree_model")
# CompileExample("t15_nodes_mocking")
# CompileExample("t16_global_blackboard")
# CompileExample("t17_blackboard_backup")
# CompileExample("t18_waypoints")

# CompileExample("ex01_wrap_legacy")
# CompileExample("ex02_runtime_ports")

# if(BTCPP_SQLITE_LOGGING)
#   CompileExample("ex03_sqlite_log")
# endif()


# ############ Create plugin and executor in folder plugin_example ##########

# # library must be SHARED
# add_library(test_plugin_action SHARED plugin_example/plugin_action.cpp )
# # you must set the definition BT_PLUGIN_EXPORT
# target_compile_definitions(test_plugin_action PRIVATE  BT_PLUGIN_EXPORT )
# # remove the "lib" prefix. Name of the file will be test_plugin_action.so
# set_target_properties(test_plugin_action PROPERTIES PREFIX "")
# # link dependencies as usual
# target_link_libraries(test_plugin_action ${BTCPP_LIBRARY} )

# add_executable(test_plugin_executor  plugin_example/plugin_executor.cpp )
# target_link_libraries(test_plugin_executor ${BTCPP_LIBRARY})
