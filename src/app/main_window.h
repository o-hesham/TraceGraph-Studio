#pragma once

#include <QMainWindow>

namespace tracegraph::app {

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
};

} // namespace tracegraph::app
