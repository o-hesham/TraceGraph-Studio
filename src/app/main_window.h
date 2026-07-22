#pragma once

#include <QMainWindow>

class QLabel;

namespace tracegraph::app
{

    class LoadController;

    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(QWidget *parent = nullptr);

    private:
        LoadController *loadController_ = nullptr;
        QLabel *sessionStatusLabel_ = nullptr;
    };

} // namespace tracegraph::app
