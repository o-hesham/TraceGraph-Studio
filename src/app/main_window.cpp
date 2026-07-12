#include "app/main_window.h"

namespace tracegraph::app {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TraceGraph Studio"));
}

} // namespace tracegraph::app
