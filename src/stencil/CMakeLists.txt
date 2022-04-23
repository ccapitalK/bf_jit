set(SOURCES
    elf_parser.cc
    main.cc
)

add_executable(stencil_compiler ${SOURCES})
target_link_libraries(stencil_compiler elf)
