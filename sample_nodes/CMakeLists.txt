# include_directories( ../include )

# # compile as static libraries

# set(CMAKE_DEBUG_POSTFIX "")

# add_library(bt_sample_nodes STATIC
#     crossdoor_nodes.cpp
#     dummy_nodes.cpp
#     movebase_node.cpp )

# target_link_libraries(bt_sample_nodes PRIVATE ${BTCPP_LIBRARY})
# set_target_properties(bt_sample_nodes PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
#     ${BTCPP_LIB_DESTINATION} )

# # to create a plugin, compile them in this way instead

# add_library(crossdoor_nodes_dyn SHARED crossdoor_nodes.cpp )
# target_link_libraries(crossdoor_nodes_dyn PRIVATE ${BTCPP_LIBRARY})
# target_compile_definitions(crossdoor_nodes_dyn PRIVATE  BT_PLUGIN_EXPORT )
# set_target_properties(crossdoor_nodes_dyn PROPERTIES LIBRARY_OUTPUT_DIRECTORY
#     ${BTCPP_BIN_DESTINATION} )

# add_library(dummy_nodes_dyn     SHARED dummy_nodes.cpp )
# target_link_libraries(dummy_nodes_dyn PRIVATE ${BTCPP_LIBRARY})
# target_compile_definitions(dummy_nodes_dyn  PRIVATE BT_PLUGIN_EXPORT)
# set_target_properties(dummy_nodes_dyn PROPERTIES LIBRARY_OUTPUT_DIRECTORY
#     ${BTCPP_BIN_DESTINATION} )


# add_library(movebase_node_dyn   SHARED movebase_node.cpp )
# target_link_libraries(movebase_node_dyn PRIVATE ${BTCPP_LIBRARY})
# target_compile_definitions(movebase_node_dyn PRIVATE  BT_PLUGIN_EXPORT )
# set_target_properties(movebase_node_dyn PROPERTIES LIBRARY_OUTPUT_DIRECTORY
#     ${BTCPP_BIN_DESTINATION} )

# ######################################################
# # INSTALL plugins for other packages to load

# INSTALL(TARGETS
#     crossdoor_nodes_dyn
#     dummy_nodes_dyn
#     movebase_node_dyn
#     LIBRARY DESTINATION share/${PROJECT_NAME}/bt_plugins
#     ARCHIVE DESTINATION share/${PROJECT_NAME}/bt_plugins
#     RUNTIME DESTINATION share/${PROJECT_NAME}/bt_plugins
# )
