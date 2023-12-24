cmake_minimum_required(VERSION 3.14)

project(m3u8_downloader LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(m3u8_downloader
  main.cpp
  variant_m3u8.h variant_m3u8.cpp
  mediaplaylist_m3u8.h mediaplaylist_m3u8.cpp
  chunk.h chunk.cpp
)
target_link_libraries(m3u8_downloader PRIVATE
    Qt6::Core
    Qt6::Network)

target_include_directories(m3u8_downloader PRIVATE ${PROJECT_SOURCE_DIR})
