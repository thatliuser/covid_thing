cmake_minimum_required(VERSION 3.8)

file(GLOB SOURCES "*.cxx")
add_executable(covid_thing "${SOURCES}")
target_compile_features(covid_thing PRIVATE cxx_std_17)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(covid_thing PRIVATE "/MT")
endif()