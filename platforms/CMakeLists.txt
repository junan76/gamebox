add_executable(gamebox main.c)
add_subdirectory(linux)
target_link_libraries(gamebox PRIVATE gb_platform)