option(ENABLE_BOOT_ROM "Enable boot rom" ON)

add_library(gb_core STATIC cpu.c bus.c device.c mbc.c ppu.c gamebox.c)
target_include_directories(gb_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(gb_core PUBLIC gb_hal)

if(ENABLE_BOOT_ROM)
    target_compile_definitions(gb_core PRIVATE USE_BOOT_ROM)
endif()