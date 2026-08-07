#include "app/main_window.h"
#include "app/load_controller.h"
#include "app/event_table_model.h"
#include "app/event_filter_proxy_model.h"
#include "app/timeline_view.h"
#include "app/selection_controller.h"
#include "app/event_inspector_widget.h"
#include "app/dependency_graph_view.h"
#include "app/filter_state.h"
#include "app/trace_summary_widget.h"
#include "domain/trace_session.h"

#include <optional>
#include <limits>

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QVariant>
#include <QItemSelectionModel>
#include <QDockWidget>
#include <QByteArray>
#include <QCloseEvent>
#include <QSettings>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>

namespace tracegraph::app
{

    MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
    {
        setWindowTitle(QStringLiteral("TraceGraph Studio"));

        resize(1200, 800);

        QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

        QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));

        QAction *openAction = fileMenu->addAction(QStringLiteral("&Open..."));
        openAction->setShortcuts(QKeySequence::Open);

        loadController_ = new LoadController(this, this);

        selectionController_ = new SelectionController(this);
        filterState_ = new FilterState(this);

        eventTableModel_ = new EventTableModel(this);

        eventFilterProxyModel_ = new EventFilterProxyModel(this);
        eventFilterProxyModel_->setSourceModel(eventTableModel_);

        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);

        filterLineEdit_ = new QLineEdit(centralWidget);
        filterLineEdit_->setPlaceholderText(QStringLiteral("Filter events..."));
        filterLineEdit_->setClearButtonEnabled(true);

        threadFilterComboBox_ = new QComboBox(centralWidget);
        threadFilterComboBox_->addItem(QStringLiteral("All threads"), QString{});

        auto *threadFilterLabel = new QLabel(QStringLiteral("&Thread:"), centralWidget);
        threadFilterLabel->setBuddy(threadFilterComboBox_);

        categoryFilterComboBox_ = new QComboBox(centralWidget);
        categoryFilterComboBox_->addItem(QStringLiteral("All categories"), QString{});

        auto *categoryFilterLabel = new QLabel(QStringLiteral("&Category:"), centralWidget);
        categoryFilterLabel->setBuddy(categoryFilterComboBox_);

        minimumDurationSpinBox_ = new QSpinBox(centralWidget);
        minimumDurationSpinBox_->setRange(0, std::numeric_limits<int>::max());
        minimumDurationSpinBox_->setSpecialValueText(QStringLiteral("Any duration"));
        minimumDurationSpinBox_->setSuffix(QStringLiteral(" \u00B5s"));
        minimumDurationSpinBox_->setKeyboardTracking(false);

        auto *minimumDurationLabel = new QLabel(QStringLiteral("&Minimum duration:"), centralWidget);
        minimumDurationLabel->setBuddy(minimumDurationSpinBox_);

        auto *filterControlsLayout = new QHBoxLayout;
        filterControlsLayout->addWidget(filterLineEdit_, 1);
        filterControlsLayout->addWidget(threadFilterLabel);
        filterControlsLayout->addWidget(threadFilterComboBox_);
        filterControlsLayout->addWidget(categoryFilterLabel);
        filterControlsLayout->addWidget(categoryFilterComboBox_);
        filterControlsLayout->addWidget(minimumDurationLabel);
        filterControlsLayout->addWidget(minimumDurationSpinBox_);

        eventTableView_ = new QTableView(centralWidget);
        eventTableView_->setModel(eventFilterProxyModel_);
        eventTableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        eventTableView_->setSelectionMode(QAbstractItemView::SingleSelection);
        eventTableView_->setAlternatingRowColors(true);
        eventTableView_->setSortingEnabled(true);
        eventTableView_->sortByColumn(EventTableModel::StartColumn, Qt::AscendingOrder);

        eventTableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        eventTableView_->horizontalHeader()->setSectionResizeMode(EventTableModel::NameColumn, QHeaderView::Stretch);

        contentSplitter_ = new QSplitter(Qt::Vertical, centralWidget);

        timelineView_ = new TimelineView;

        contentSplitter_->addWidget(eventTableView_);
        contentSplitter_->addWidget(timelineView_);
        contentSplitter_->setChildrenCollapsible(false);
        contentSplitter_->setStretchFactor(0, 1);
        contentSplitter_->setStretchFactor(1, 1);

        centralLayout->addLayout(filterControlsLayout);
        centralLayout->addWidget(contentSplitter_);

        setCentralWidget(centralWidget);

        eventInspectorDock_ = new QDockWidget(QStringLiteral("Event Inspector"), this);
        eventInspectorDock_->setObjectName(QStringLiteral("EventInspectorDock"));
        eventInspectorDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        eventInspectorWidget_ = new EventInspectorWidget(eventInspectorDock_);
        eventInspectorDock_->setWidget(eventInspectorWidget_);

        addDockWidget(Qt::RightDockWidgetArea, eventInspectorDock_);

        traceSummaryDock_ = new QDockWidget(QStringLiteral("Trace Summary"), this);
        traceSummaryDock_->setObjectName(QStringLiteral("TraceSummaryDock"));
        traceSummaryDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        traceSummaryWidget_ = new TraceSummaryWidget(traceSummaryDock_);
        traceSummaryDock_->setWidget(traceSummaryWidget_);

        addDockWidget(Qt::RightDockWidgetArea, traceSummaryDock_);
        tabifyDockWidget(eventInspectorDock_, traceSummaryDock_);
        traceSummaryDock_->raise();

        dependencyGraphDock_ = new QDockWidget(QStringLiteral("Dependency Graph"), this);
        dependencyGraphDock_->setObjectName(QStringLiteral("DependencyGraphDock"));
        dependencyGraphDock_->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);

        dependencyGraphView_ = new DependencyGraphView(dependencyGraphDock_);
        dependencyGraphDock_->setWidget(dependencyGraphView_);

        addDockWidget(Qt::BottomDockWidgetArea, dependencyGraphDock_);

        viewMenu->addAction(eventInspectorDock_->toggleViewAction());
        viewMenu->addAction(traceSummaryDock_->toggleViewAction());
        viewMenu->addAction(dependencyGraphDock_->toggleViewAction());

        sessionStatusLabel_ = new QLabel(QStringLiteral("No trace loaded"), statusBar());
        statusBar()->addPermanentWidget(sessionStatusLabel_);

        connect(openAction, &QAction::triggered, loadController_, &LoadController::openTraceFile);
        connect(loadController_, &LoadController::sessionLoaded, this,
                [this](const domain::TraceSession *session)
                {
                    if (session == nullptr)
                    {
                        return;
                    }

                    selectionController_->clearSelection();

                    rebuildFilterOptions(*session);

                    eventTableModel_->setSession(session);
                    timelineView_->setSession(session);
                    eventInspectorWidget_->setSession(session);
                    traceSummaryWidget_->setSession(session);
                    dependencyGraphView_->setSession(session);

                    sessionStatusLabel_->setText(QStringLiteral("Loaded %1 events.").arg(session->events().size()));
                });
        connect(loadController_, &LoadController::loadingChanged, this,
                [this, openAction](bool isLoading)
                {
                    openAction->setEnabled(!isLoading);

                    if (isLoading)
                    {
                        sessionStatusLabel_->setText(QStringLiteral("Loading trace..."));
                        return;
                    }

                    const domain::TraceSession *session = loadController_->session();

                    if (session == nullptr)
                    {
                        sessionStatusLabel_->setText(QStringLiteral("No trace loaded"));
                        return;
                    }

                    sessionStatusLabel_->setText(QStringLiteral("Loaded %1 events.").arg(session->events().size()));
                });
        connect(filterLineEdit_, &QLineEdit::textChanged, filterState_, &FilterState::setFilterText);
        connect(filterState_, &FilterState::filterTextChanged, eventFilterProxyModel_, &EventFilterProxyModel::setFilterFixedString);
        connect(filterState_, &FilterState::filterTextChanged, timelineView_, &TimelineView::setFilterText);
        connect(eventTableView_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this](const QItemSelection &, const QItemSelection &)
                {
                    const QModelIndexList selectedRows = eventTableView_->selectionModel()->selectedRows(
                        EventTableModel::IdColumn);

                    if (selectedRows.isEmpty())
                    {
                        selectionController_->clearSelection();
                        return;
                    }

                    const domain::EventId eventId = selectedRows.first().data(EventTableModel::EventIdRole).toLongLong();

                    selectionController_->selectEvent(eventId);
                });
        connect(timelineView_, &TimelineView::eventSelected, selectionController_, &SelectionController::selectEvent);
        connect(timelineView_, &TimelineView::selectionCleared, selectionController_, &SelectionController::clearSelection);
        connect(selectionController_, &SelectionController::selectedEventIdChanged, timelineView_, &TimelineView::setSelectedEventId);
        connect(selectionController_, &SelectionController::selectedEventIdChanged, this,
                [this](std::optional<domain::EventId> selectedEventId)
                {
                    if (!selectedEventId.has_value())
                    {
                        eventTableView_->clearSelection();
                        return;
                    }

                    const QModelIndexList sourceMatches = eventTableModel_->match(
                        eventTableModel_->index(
                            0, EventTableModel::IdColumn),
                        EventTableModel::EventIdRole,
                        QVariant::fromValue(*selectedEventId),
                        1,
                        Qt::MatchExactly);

                    if (sourceMatches.isEmpty())
                    {
                        eventTableView_->clearSelection();
                        return;
                    }

                    const QModelIndex proxyIndex = eventFilterProxyModel_->mapFromSource(sourceMatches.first());

                    if (!proxyIndex.isValid())
                    {
                        eventTableView_->clearSelection();
                        return;
                    }

                    eventTableView_->selectRow(proxyIndex.row());
                    eventTableView_->scrollTo(proxyIndex);
                });
        connect(selectionController_, &SelectionController::selectedEventIdChanged, eventInspectorWidget_, &EventInspectorWidget::setSelectedEventId);
        connect(selectionController_, &SelectionController::selectedEventIdChanged, dependencyGraphView_, &DependencyGraphView::setSelectedEventId);
        connect(dependencyGraphView_, &DependencyGraphView::eventSelected, selectionController_, &SelectionController::selectEvent);
        connect(minimumDurationSpinBox_, &QSpinBox::valueChanged, filterState_,
                [this](int value)
                {
                    filterState_->setMinimumDurationMicroseconds(static_cast<qint64>(value));
                });
        connect(filterState_, &FilterState::minimumDurationMicrosecondsChanged, eventFilterProxyModel_, &EventFilterProxyModel::setMinimumDurationMicroseconds);
        connect(filterState_, &FilterState::minimumDurationMicrosecondsChanged, timelineView_, &TimelineView::setMinimumDurationMicroseconds);
        connect(threadFilterComboBox_, &QComboBox::currentIndexChanged, filterState_,
                [this](int index)
                {
                    const QString threadFilter =
                        threadFilterComboBox_->itemData(index).toString();

                    filterState_->setThreadFilter(threadFilter);
                });
        connect(filterState_, &FilterState::threadFilterChanged, eventFilterProxyModel_, &EventFilterProxyModel::setThreadFilter);
        connect(filterState_, &FilterState::threadFilterChanged, timelineView_, &TimelineView::setThreadFilter);
        connect(categoryFilterComboBox_, &QComboBox::currentIndexChanged, filterState_,
                [this](int index)
                {
                    const QString categoryFilter =
                        categoryFilterComboBox_->itemData(index).toString();

                    filterState_->setCategoryFilter(categoryFilter);
                });
        connect(filterState_, &FilterState::categoryFilterChanged, eventFilterProxyModel_, &EventFilterProxyModel::setCategoryFilter);
        connect(filterState_, &FilterState::categoryFilterChanged, timelineView_, &TimelineView::setCategoryFilter);

        openAction->setEnabled(true);

        restoreWindowSettings();
    }

    void MainWindow::rebuildFilterOptions(const domain::TraceSession &session)
    {
        QSet<QString> uniqueThreadNames;
        QSet<QString> uniqueCategoryNames;

        for (const auto &event : session.events())
        {
            uniqueThreadNames.insert(event.thread);
            uniqueCategoryNames.insert(event.category);
        }

        QStringList threadNames;
        threadNames.reserve(uniqueThreadNames.size());

        for (const auto &threadName : uniqueThreadNames)
        {
            threadNames.append(threadName);
        }

        threadNames.sort(Qt::CaseInsensitive);

        QStringList categoryNames;
        categoryNames.reserve(uniqueCategoryNames.size());

        for (const QString &categoryName : uniqueCategoryNames)
        {
            categoryNames.append(categoryName);
        }

        categoryNames.sort(Qt::CaseInsensitive);

        // Avoid emitting temporary selection changes while rebuilding the controls.
        const QSignalBlocker threadSignalBlocker(threadFilterComboBox_);
        const QSignalBlocker categorySignalBlocker(categoryFilterComboBox_);

        //////////////////////////////////////////////////////////////
        // Rebuild choices for the thread on which an event executed.
        //////////////////////////////////////////////////////////////
        threadFilterComboBox_->clear();
        threadFilterComboBox_->addItem(QStringLiteral("All threads"), QString{});

        for (const QString &threadName : threadNames)
        {
            threadFilterComboBox_->addItem(threadName, threadName);
        }

        threadFilterComboBox_->setCurrentIndex(0);

        //////////////////////////////////////////////////////////////
        // Rebuild choices for the type of work represented by an event.
        //////////////////////////////////////////////////////////////
        categoryFilterComboBox_->clear();
        categoryFilterComboBox_->addItem(
            QStringLiteral("All categories"),
            QString{});

        for (const QString &categoryName : categoryNames)
        {
            categoryFilterComboBox_->addItem(
                categoryName,
                categoryName);
        }

        categoryFilterComboBox_->setCurrentIndex(0);

        // A new trace starts with both structured filters disabled.
        filterState_->setThreadFilter(QString{});
        filterState_->setCategoryFilter(QString{});
    }

    void MainWindow::restoreWindowSettings()
    {
        QSettings settings;

        const QByteArray geometry = settings.value(QStringLiteral("mainWindow/geometry")).toByteArray();
        const QByteArray windowState = settings.value(QStringLiteral("mainWindow/state")).toByteArray();
        const QByteArray splitterState = settings.value(QStringLiteral("mainWindow/contentSplitter")).toByteArray();

        if (!geometry.isEmpty())
        {
            restoreGeometry(geometry);
        }

        if (!windowState.isEmpty())
        {
            restoreState(windowState);
        }

        if (!splitterState.isEmpty())
        {
            contentSplitter_->restoreState(splitterState);
        }
    }

    void MainWindow::saveWindowSettings() const
    {
        QSettings settings;

        settings.setValue(QStringLiteral("mainWindow/geometry"), saveGeometry());
        settings.setValue(QStringLiteral("mainWindow/state"), saveState());
        settings.setValue(QStringLiteral("mainWindow/contentSplitter"), contentSplitter_->saveState());
    }

    void MainWindow::closeEvent(QCloseEvent *event)
    {
        saveWindowSettings();
        QMainWindow::closeEvent(event);
    }

} // namespace tracegraph::app
