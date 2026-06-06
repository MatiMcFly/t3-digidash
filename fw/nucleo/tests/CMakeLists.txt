cmake_minimum_required(VERSION 3.22)

project(t3_digidash_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(unity)

add_library(unity_framework STATIC
    ${unity_SOURCE_DIR}/src/unity.c
)

target_include_directories(unity_framework PUBLIC
    ${unity_SOURCE_DIR}/src
)

enable_testing()

add_executable(conversion_tests
    conversion_tests.c
    ../CM4/app/conversion/conversion.c
)

target_include_directories(conversion_tests PRIVATE
    ../CM4/app/conversion
)

if(NOT MSVC)
    target_link_libraries(conversion_tests PRIVATE unity_framework m)
else()
    target_link_libraries(conversion_tests PRIVATE unity_framework)
endif()

add_test(NAME conversion_tests COMMAND conversion_tests)

add_executable(filtering_tests
    filtering_tests.c
    ../CM4/app/filtering/filtering.c
)

target_include_directories(filtering_tests PRIVATE
    ../CM4/app/filtering
)

target_compile_definitions(filtering_tests PRIVATE
    UNIT_TEST
)

if(NOT MSVC)
    target_link_libraries(filtering_tests PRIVATE unity_framework)
else()
    target_link_libraries(filtering_tests PRIVATE unity_framework)
endif()

add_test(NAME filtering_tests COMMAND filtering_tests)
