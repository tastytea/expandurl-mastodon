cmake_minimum_required (VERSION 3.7)
project (expandurl-mastodon
         VERSION 0.3.2
         LANGUAGES CXX
)

include(GNUInstallDirs)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURLPP REQUIRED curlpp)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CURLPP_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIRS})

link_directories(${CURL_LIBRARY_DIRS})
link_directories(${CURLPP_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})

# Write version in header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_BINARY_DIR}/version.hpp"
)

file(GLOB sources src/*.cpp)
add_executable(expandurl-mastodon ${sources})
target_link_libraries(expandurl-mastodon
                      ${CURLPP_LIBRARIES} ${JSONCPP_LIBRARIES}
                      mastodon-cpp pthread)
install(TARGETS expandurl-mastodon DESTINATION ${CMAKE_INSTALL_BINDIR})
