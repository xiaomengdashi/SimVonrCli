cmake_minimum_required(VERSION 3.20)

project(sim_vonr_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(yaml-cpp REQUIRED)
include(CTest)

set(DOUBANGO_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/doubango/build/install")
set(DOUBANGO_INCLUDE_DIR "${DOUBANGO_INSTALL_DIR}/include")
set(DOUBANGO_LIB_DIR "${DOUBANGO_INSTALL_DIR}/lib")

add_library(sim_config
    src/config/config_loader.cpp
)

add_library(sim_ims
    src/ims/ims_header_policy.cpp
)

add_library(sim_auth
    src/auth/ims_auth_service.cpp
)

add_library(sim_core
    src/core/sip_stack_adapter.cpp
)

add_library(sim_registration
    src/registration/registration_service.cpp
)

add_library(sim_call
    src/call/call_service.cpp
)

add_library(sim_cli
    src/cli/cli_controller.cpp
)

add_library(sim_util
    src/util/redaction.cpp
)

target_include_directories(sim_config
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_ims
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_auth
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${DOUBANGO_INCLUDE_DIR}
        ${DOUBANGO_INCLUDE_DIR}/tinysip
        ${DOUBANGO_INCLUDE_DIR}/tinysak
        ${DOUBANGO_INCLUDE_DIR}/tinynet
        ${DOUBANGO_INCLUDE_DIR}/tinymedia
        ${DOUBANGO_INCLUDE_DIR}/tinysdp
)

target_include_directories(sim_registration
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_call
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_cli
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(sim_util
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sim_config
    PUBLIC
        yaml-cpp
)

target_link_libraries(sim_core
    PUBLIC
        sim_auth
        ${DOUBANGO_LIB_DIR}/libtinySIP.a
        ${DOUBANGO_LIB_DIR}/libtinyMEDIA.a
        ${DOUBANGO_LIB_DIR}/libtinyDAV.a
        ${DOUBANGO_LIB_DIR}/libtinyRTP.a
        ${DOUBANGO_LIB_DIR}/libtinyHTTP.a
        ${DOUBANGO_LIB_DIR}/libtinySDP.a
        ${DOUBANGO_LIB_DIR}/libtinyNET.a
        ${DOUBANGO_LIB_DIR}/libtinySIGCOMP.a
        ${DOUBANGO_LIB_DIR}/libtinyIPSec.a
        ${DOUBANGO_LIB_DIR}/libtinySAK.a
        ssl
        crypto
        pthread
        dl
        m
)

add_executable(sim_vonr_cli
    src/main.cpp
)

target_link_libraries(sim_vonr_cli
    PRIVATE
        sim_cli
        sim_registration
        sim_call
        sim_config
        sim_core
)

target_include_directories(sim_vonr_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(config_loader_test
        tests/config/config_loader_test.cpp
    )

    add_executable(ims_header_policy_test
        tests/ims/ims_header_policy_test.cpp
    )

    add_executable(ims_auth_service_test
        tests/auth/ims_auth_service_test.cpp
    )

    add_executable(sip_stack_adapter_test
        tests/core/sip_stack_adapter_test.cpp
    )

    add_executable(registration_service_test
        tests/registration/registration_service_test.cpp
    )

    add_executable(call_service_test
        tests/call/call_service_test.cpp
    )

    add_executable(cli_controller_test
        tests/cli/cli_controller_test.cpp
    )

    add_executable(redaction_test
        tests/util/redaction_test.cpp
    )

    add_executable(sip_flow_smoke_test
        tests/integration/sip_flow_smoke_test.cpp
    )

    target_link_libraries(config_loader_test
        PRIVATE
            sim_config
            GTest::gtest_main
    )

    target_link_libraries(ims_header_policy_test
        PRIVATE
            sim_ims
            GTest::gtest_main
    )

    target_link_libraries(ims_auth_service_test
        PRIVATE
            sim_auth
            GTest::gtest_main
    )

    target_link_libraries(sip_stack_adapter_test
        PRIVATE
            sim_core
            GTest::gtest_main
    )

    target_link_libraries(registration_service_test
        PRIVATE
            sim_registration
            GTest::gtest_main
    )

    target_link_libraries(call_service_test
        PRIVATE
            sim_call
            GTest::gtest_main
    )

    target_link_libraries(cli_controller_test
        PRIVATE
            sim_cli
            GTest::gtest_main
    )

    target_link_libraries(redaction_test
        PRIVATE
            sim_util
            GTest::gtest_main
    )

    target_link_libraries(sip_flow_smoke_test
        PRIVATE
            sim_registration
            sim_call
            GTest::gtest_main
    )

    target_compile_definitions(config_loader_test
        PRIVATE
            TEST_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    gtest_discover_tests(config_loader_test)
    gtest_discover_tests(ims_header_policy_test)
    gtest_discover_tests(ims_auth_service_test)
    gtest_discover_tests(sip_stack_adapter_test)
    gtest_discover_tests(registration_service_test)
    gtest_discover_tests(call_service_test)
    gtest_discover_tests(cli_controller_test)
    gtest_discover_tests(redaction_test)
    gtest_discover_tests(sip_flow_smoke_test)
endif()
