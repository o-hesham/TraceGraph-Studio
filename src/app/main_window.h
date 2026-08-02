#pragma once

#include <QMainWindow>

class QLabel;
class QTableView;
class QLineEdit;
class QSplitter;
class QDockWidget;
class QCloseEvent;

namespace tracegraph::app
{

    class LoadController;
    class EventTableModel;
    class EventFilterProxyModel;
    class TimelineView;
    class SelectionController;
    class EventInspectorWidget;
    class DependencyGraphView;
    class FilterState;

    class MainWindow final : public QMainWindow
    {
    public:
        explicit MainWindow(QWidget *parent = nullptr);

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        // Restores the previously saved window and splitter layout.
        void restoreWindowSettings();

        // Saves the current window and splitter layout.
        void saveWindowSettings() const;

        LoadController *loadController_ = nullptr;
        QLabel *sessionStatusLabel_ = nullptr;

        EventTableModel *eventTableModel_ = nullptr;
        QTableView *eventTableView_ = nullptr;

        EventFilterProxyModel *eventFilterProxyModel_ = nullptr;

        QLineEdit *filterLineEdit_ = nullptr;
        QSplitter *contentSplitter_ = nullptr;

        TimelineView *timelineView_ = nullptr;

        SelectionController *selectionController_ = nullptr;
        FilterState *filterState_ = nullptr;

        QDockWidget *eventInspectorDock_ = nullptr;
        EventInspectorWidget *eventInspectorWidget_ = nullptr;

        QDockWidget *dependencyGraphDock_ = nullptr;
        DependencyGraphView *dependencyGraphView_ = nullptr;
    };

} // namespace tracegraph::app
