add_executable(gamebox main.c)
find_package(raylib REQUIRED)
target_link_libraries(gamebox PRIVATE gb_core raylib)