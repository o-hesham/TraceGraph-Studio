qt_add_library(tracegraph_app STATIC
    app/main_window.cpp
)

target_include_directories(tracegraph_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tracegraph_app
    PUBLIC
        Qt6::Widgets
)

qt_add_executable(tracegraph-studio
    main.cpp
)

target_link_libraries(tracegraph-studio
    PRIVATE
        tracegraph_app
)

set_target_properties(tracegraph-studio PROPERTIES
    WIN32_EXECUTABLE ON
)
