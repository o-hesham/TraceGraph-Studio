qt_add_executable(main_window_test
    app/main_window_test.cpp
)

target_link_libraries(main_window_test
    PRIVATE
        tracegraph_app
        Qt6::Test
)

add_test(NAME main_window_test COMMAND main_window_test)

set_tests_properties(main_window_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
