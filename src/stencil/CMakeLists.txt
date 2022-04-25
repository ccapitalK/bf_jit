project(stencil_compiler)

set(SOURCES
    elf_parser.cc
    main.cc
)

add_executable(stencil_compiler ${SOURCES})
target_link_libraries(stencil_compiler elf)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND g++ -c -o stencil_library.o ${CMAKE_CURRENT_SOURCE_DIR}/stencil_library.cc -fno-stack-protector -falign-functions=1 -O2)