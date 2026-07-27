#include "app/main_window.h"
#include "app/load_controller.h"
#include "app/event_table_model.h"
#include "app/event_filter_proxy_model.h"
#include "app/timeline_view.h"
#include "app/selection_controller.h"

#include <optional>

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

namespace tracegraph::app
{

    MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
    {
        setWindowTitle(QStringLiteral("TraceGraph Studio"));

        QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

        QAction *openAction = fileMenu->addAction(QStringLiteral("&Open..."));
        openAction->setShortcuts(QKeySequence::Open);

        loadController_ = new LoadController(this, this);

        selectionController_ = new SelectionController(this);

        eventTableModel_ = new EventTableModel(this);

        eventFilterProxyModel_ = new EventFilterProxyModel(this);
        eventFilterProxyModel_->setSourceModel(eventTableModel_);

        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);

        filterLineEdit_ = new QLineEdit(centralWidget);
        filterLineEdit_->setPlaceholderText(QStringLiteral("Filter events..."));
        filterLineEdit_->setClearButtonEnabled(true);

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

        centralLayout->addWidget(filterLineEdit_);
        centralLayout->addWidget(contentSplitter_);

        setCentralWidget(centralWidget);

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
                    eventTableModel_->setSession(session);
                    timelineView_->setSession(session);

                    sessionStatusLabel_->setText(QStringLiteral("Loaded %1 events.").arg(session->events().size()));
                });
        connect(filterLineEdit_, &QLineEdit::textChanged, eventFilterProxyModel_, &QSortFilterProxyModel::setFilterFixedString);
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

        openAction->setEnabled(true);
    }

} // namespace tracegraph::app
