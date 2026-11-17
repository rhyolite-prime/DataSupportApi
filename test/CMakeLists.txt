cmake_minimum_required(VERSION 3.10)
project(DataSupportApi_test CXX)

add_executable(${PROJECT_NAME} test_main.cc magma_net_test.cc
        ../utils/HmacUtils.cc)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

ParseAndAddDrogonTests(${PROJECT_NAME})
