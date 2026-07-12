add_library(gb_platform STATIC linux.c)
find_package(raylib REQUIRED)

target_include_directories(gb_platform PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gb_platform PRIVATE gb_core raylib)