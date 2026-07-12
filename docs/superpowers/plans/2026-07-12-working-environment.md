# TraceGraph Studio Working Environment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a reproducible Qt 6 Widgets application and Qt Test harness that builds with the installed MinGW, CMake, and Ninja toolchain.

**Architecture:** A top-level CMake file owns project-wide language, Qt, output-directory, and testing settings. Application code lives in a small static library so the production executable and tests exercise the same `MainWindow` implementation. The first Qt Test runs with the offscreen platform plugin so CTest does not require an interactive display.

**Tech Stack:** C++20, Qt 6.11.1 Widgets and Test, CMake 3.30.5, Ninja 1.12.1, MinGW-w64 GCC 13.1, CTest, Windows PowerShell.

## Global Constraints

- Product name: `TraceGraph Studio`.
- CMake project name: `TraceGraphStudio`.
- Executable name: `tracegraph-studio`.
- Use C++20 with compiler extensions disabled.
- Use only Qt 6.11.1 modules already installed under `D:/Qt/6.11.1/mingw_64`.
- Keep build artifacts under `build/`; never commit them.
- Production application code must be reusable by tests; do not duplicate `MainWindow` behavior in test code.
- This plan implements only the working environment and minimal window/test harness. Trace-domain code belongs to the next milestone.

---

## Planned File Structure

```text
TraceGraph Studio/
├── .gitignore                         # Excludes generated build output
├── CMakeLists.txt                     # Project-wide Qt and CTest configuration
├── src/
│   ├── CMakeLists.txt                 # Application library and executable targets
│   ├── main.cpp                       # QApplication entry point
│   └── app/
│       ├── main_window.h              # MainWindow public interface
│       └── main_window.cpp            # MainWindow behavior
└── tests/
    ├── CMakeLists.txt                 # Qt Test target and CTest registration
    └── app/
        └── main_window_test.cpp       # Initial window-state test
```

`tracegraph_app` is a static library rather than a second executable. This lets `tracegraph-studio` and `main_window_test` link to the exact same application code.

### Task 1: Create and verify the Qt application/test harness

**Files:**

- Create: `.gitignore`
- Create: `CMakeLists.txt`
- Create: `src/CMakeLists.txt`
- Create: `src/main.cpp`
- Create: `src/app/main_window.h`
- Create: `src/app/main_window.cpp`
- Create: `tests/CMakeLists.txt`
- Create: `tests/app/main_window_test.cpp`

**Interfaces:**

- Consumes: Qt targets `Qt6::Widgets` and `Qt6::Test` from the installed Qt 6.11.1 package.
- Produces: `tracegraph::app::MainWindow::MainWindow(QWidget *parent = nullptr)`.
- Produces: CMake library target `tracegraph_app` for later application components and tests.
- Produces: executable target `tracegraph-studio` in `build/bin/`.
- Produces: CTest test `main_window_test` in `build/bin/`.

- [ ] **Step 1: Add the build-output ignore rule**

Create `.gitignore`:

```gitignore
/.worktrees/
/build/
```

Why: local worktrees and CMake/Ninja output are machine-specific. Git should contain sources and configuration, not isolated checkouts or generated output.

- [ ] **Step 2: Create the project-wide CMake configuration**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)

project(TraceGraphStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets Test)
qt_standard_project_setup(REQUIRES 6.11)

include(CTest)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

Why: the top-level file defines rules shared by every future target. `qt_standard_project_setup()` enables supported Qt defaults such as automatic Meta-Object Compiler handling before targets are created.

- [ ] **Step 3: Create the application target structure**

Create `src/CMakeLists.txt`:

```cmake
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
```

Why: target-based linking makes Qt include paths and compiler settings travel with the target that needs them. The executable depends on the reusable application library instead of compiling application classes separately.

- [ ] **Step 4: Add a compilable MainWindow shell without the required title**

Create `src/app/main_window.h`:

```cpp
#pragma once

#include <QMainWindow>

namespace tracegraph::app {

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
};

} // namespace tracegraph::app
```

Create `src/app/main_window.cpp`:

```cpp
#include "app/main_window.h"

namespace tracegraph::app {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
}

} // namespace tracegraph::app
```

Why: this is the minimal production shell required to compile the test. It deliberately omits the intended window title so the first test can demonstrate a real red-to-green cycle.

- [ ] **Step 5: Add the application entry point**

Create `src/main.cpp`:

```cpp
#include "app/main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    tracegraph::app::MainWindow window;
    window.resize(1200, 800);
    window.show();

    return application.exec();
}
```

Why: `QApplication` owns the GUI event loop. No window can process painting or input until `application.exec()` starts that loop.

- [ ] **Step 6: Write the failing Qt Test**

Create `tests/app/main_window_test.cpp`:

```cpp
#include "app/main_window.h"

#include <QTest>

class MainWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void hasProductTitle();
};

void MainWindowTest::hasProductTitle()
{
    tracegraph::app::MainWindow window;

    QCOMPARE(window.windowTitle(), QStringLiteral("TraceGraph Studio"));
}

QTEST_MAIN(MainWindowTest)

#include "main_window_test.moc"
```

Why: a Qt Test class inherits `QObject`, declares test functions as private slots, and uses `QTEST_MAIN` to create the required application object and run the test.

- [ ] **Step 7: Register the test with CTest**

Create `tests/CMakeLists.txt`:

```cmake
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
```

Why: CTest provides one command for the entire future test suite. The offscreen Qt platform plugin lets this widget test create a window without displaying it.

- [ ] **Step 8: Configure the project with the verified local toolchain**

Run from PowerShell in `D:\projects\TraceGraph Studio`:

```powershell
$env:Path = 'D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path

& 'D:\Qt\Tools\CMake_64\bin\cmake.exe' `
    -S . `
    -B build `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_PREFIX_PATH='D:/Qt/6.11.1/mingw_64' `
    -DCMAKE_MAKE_PROGRAM='D:/Qt/Tools/Ninja/ninja.exe'
```

Expected: configuration ends with both `Configuring done` and `Generating done`, and writes build files to `build/`.

- [ ] **Step 9: Build and run the test to prove it fails for the intended reason**

Run:

```powershell
$env:Path = 'D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path

& 'D:\Qt\Tools\CMake_64\bin\cmake.exe' --build build --target main_window_test
& 'D:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure -R '^main_window_test$'
```

Expected: compilation and linking succeed, but CTest reports one failed test because the actual title is empty while the expected title is `TraceGraph Studio`.

- [ ] **Step 10: Implement the minimum behavior that satisfies the test**

Replace `src/app/main_window.cpp` with:

```cpp
#include "app/main_window.h"

namespace tracegraph::app {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TraceGraph Studio"));
}

} // namespace tracegraph::app
```

Why: the smallest correct implementation sets only the behavior required by the test. Additional UI belongs to later milestones.

- [ ] **Step 11: Build everything and prove the test passes**

Run:

```powershell
$env:Path = 'D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path

& 'D:\Qt\Tools\CMake_64\bin\cmake.exe' --build build
& 'D:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure
```

Expected: the build succeeds and CTest reports `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 12: Launch the application for a visual smoke check**

Run:

```powershell
$env:Path = 'D:\Qt\6.11.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:Path

& '.\build\bin\tracegraph-studio.exe'
```

Expected: a resizable empty window titled `TraceGraph Studio` opens at 1200 by 800 pixels. Close the window to return to PowerShell.

- [ ] **Step 13: Check repository cleanliness and commit the milestone**

Run:

```powershell
git status --short
git diff --check
git add .gitignore CMakeLists.txt src tests
git commit -m "build: add Qt application and test harness"
git status --short
```

Expected: `build/` does not appear in the staged files, the commit succeeds, and the final status output is empty.

## Evidence References

- Qt's CMake guide documents `find_package(Qt6)`, `qt_standard_project_setup()`, target-based linking, `qt_add_executable()`, and subdirectory project structure: <https://doc.qt.io/qt-6/cmake-get-started.html>
- Qt documents that `qt_standard_project_setup()` should run after `find_package()` and before targets: <https://doc.qt.io/qt-6/qt-standard-project-setup.html>
- Qt Test documents QObject-based test classes, private-slot test functions, `QTEST_MAIN`, CMake targets, and CTest registration: <https://doc.qt.io/qt-6/qtest-overview.html>
