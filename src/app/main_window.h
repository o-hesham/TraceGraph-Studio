#pragma once

#include <QMainWindow>

class QLabel;
class QTableView;

namespace tracegraph::app
{

    class LoadController;
    class EventTableModel;

    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(QWidget *parent = nullptr);

    private:
        LoadController *loadController_ = nullptr;
        QLabel *sessionStatusLabel_ = nullptr;
        EventTableModel *eventTableModel_ = nullptr;
        QTableView *eventTableView_ = nullptr;
    };

} // namespace tracegraph::app
