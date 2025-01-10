# ######################################################
# # TESTS

# set(BT_TESTS
#   src/action_test_node.cpp
#   src/condition_test_node.cpp

#   gtest_any.cpp
#   gtest_blackboard.cpp
#   gtest_coroutines.cpp
#   gtest_decorator.cpp
#   gtest_enums.cpp
#   gtest_factory.cpp
#   gtest_fallback.cpp
#   gtest_parallel.cpp
#   gtest_preconditions.cpp
#   gtest_ports.cpp
#   gtest_postconditions.cpp
#   gtest_match.cpp
#   gtest_json.cpp
#   gtest_reactive.cpp
#   gtest_reactive_backchaining.cpp
#   gtest_sequence.cpp
#   gtest_skipping.cpp
#   gtest_substitution.cpp
#   gtest_subtree.cpp
#   gtest_switch.cpp
#   gtest_tree.cpp
#   gtest_updates.cpp
#   gtest_wakeup.cpp

#   script_parser_test.cpp
#   test_helper.hpp
# )

# set(TEST_DEPENDECIES
#     ${BTCPP_LIBRARY}
#     foonathan::lexy
#     bt_sample_nodes)

# if(ament_cmake_FOUND AND BUILD_TESTING)

#     find_package(ament_cmake_gtest REQUIRED)

#     ament_add_gtest(${BTCPP_LIBRARY}_test ${BT_TESTS})
#     target_link_libraries(${BTCPP_LIBRARY}_test
#         ${TEST_DEPENDECIES}
#         ${ament_LIBRARIES})

# elseif(catkin_FOUND AND CATKIN_ENABLE_TESTING)

#     catkin_add_gtest(${BTCPP_LIBRARY}_test ${BT_TESTS})
#     target_link_libraries(${BTCPP_LIBRARY}_test
#         ${TEST_DEPENDECIES}
#         Threads::Threads
#         ${catkin_LIBRARIES})

# else()

#     find_package(GTest REQUIRED)
#     enable_testing()

#     add_executable(${BTCPP_LIBRARY}_test ${BT_TESTS})

#     target_link_libraries(${PROJECT_NAME}_test
#         ${TEST_DEPENDECIES}
#         Threads::Threads
#         GTest::gtest)

# endif()

# target_include_directories(${BTCPP_LIBRARY}_test PRIVATE include ${PROJECT_SOURCE_DIR}/3rdparty)
# target_compile_definitions(${BTCPP_LIBRARY}_test PRIVATE BT_TEST_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}")
