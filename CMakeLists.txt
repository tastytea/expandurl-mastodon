cmake_minimum_required (VERSION 3.7)
project (expandurl-mastodon
         VERSION 0.1.0
         LANGUAGES CXX
)

include(GNUInstallDirs)
include(FindCURL)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

# Write version in header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_BINARY_DIR}/version.hpp"
)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

file(GLOB sources src/*.cpp)
add_executable(expandurl-mastodon ${sources})
target_link_libraries(expandurl-mastodon jsoncpp mastodon-cpp curl curlpp pthread)
install(TARGETS expandurl-mastodon DESTINATION ${CMAKE_INSTALL_BINDIR})
