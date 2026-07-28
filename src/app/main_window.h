#pragma once

#include <QMainWindow>

class QLabel;
class QTableView;
class QLineEdit;
class QSplitter;
class QDockWidget;

namespace tracegraph::app
{

    class LoadController;
    class EventTableModel;
    class EventFilterProxyModel;
    class TimelineView;
    class SelectionController;
    class EventInspectorWidget;

    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(QWidget *parent = nullptr);

    private:
        LoadController *loadController_ = nullptr;
        QLabel *sessionStatusLabel_ = nullptr;

        EventTableModel *eventTableModel_ = nullptr;
        QTableView *eventTableView_ = nullptr;

        EventFilterProxyModel *eventFilterProxyModel_ = nullptr;

        QLineEdit *filterLineEdit_ = nullptr;
        QSplitter *contentSplitter_ = nullptr;

        TimelineView *timelineView_ = nullptr;

        SelectionController *selectionController_ = nullptr;

        QDockWidget *eventInspectorDock_ = nullptr;
        EventInspectorWidget *eventInspectorWidget_ = nullptr;
    };

} // namespace tracegraph::app
