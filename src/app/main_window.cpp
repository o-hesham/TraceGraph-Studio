#include "app/main_window.h"
#include "app/load_controller.h"
#include "app/event_table_model.h"
#include "app/event_filter_proxy_model.h"

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

        centralLayout->addWidget(filterLineEdit_);
        centralLayout->addWidget(eventTableView_);

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

                    eventTableModel_->setSession(session);

                    sessionStatusLabel_->setText(QStringLiteral("Loaded %1 events.").arg(session->events().size()));
                });
        connect(filterLineEdit_, &QLineEdit::textChanged, eventFilterProxyModel_, &QSortFilterProxyModel::setFilterFixedString);

        openAction->setEnabled(true);
    }

} // namespace tracegraph::app
