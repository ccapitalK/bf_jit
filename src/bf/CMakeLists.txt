set(SOURCES
    arguments.cc
    asmbuf.cc
    code_generator.cc
    engine.cc
    interpreter.cc
    ir.cc
    main.cc
    optimizer.cc
    parser.cc
    runtime.cc
)

add_executable(bf ${SOURCES})