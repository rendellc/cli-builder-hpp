cmake_minimum_required(VERSION 3.5)


project(cli_builder VERSION 0.0.1)

include_directories(include)


file(GLOB EXAMPLES "examples/*.cpp")
foreach(EXAMPLE_SRC ${EXAMPLES})
  # remove extension for name
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})
endforeach()


# add_executable(multiple_commands examples/multiple_commands.cpp)


include(FetchContent)
message("Fetching Catch2")
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(tests tests/cli_tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
include(Catch)

# # find_package(Catch2 REQUIRED)
# add_executable(tests tests/lib_tests.cpp)
# target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
# 
# include(Catch)

# include(FetchContent)
# FetchContent_Declare(
# 	Catch2
# 	GIT_REPOSITORY https://github.com/catchorg/Catch2
# 	GIT_TAG v3.4.0
# )
# 
# FetchContent_MakeAvailable(Catch2)
# list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
