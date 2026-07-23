#include "app/main_window.h"
#include "app/load_controller.h"
#include "app/event_table_model.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>

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

        eventTableView_ = new QTableView(this);
        eventTableView_->setModel(eventTableModel_);

        eventTableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        eventTableView_->setSelectionMode(QAbstractItemView::SingleSelection);
        eventTableView_->setAlternatingRowColors(true);

        eventTableView_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        eventTableView_->horizontalHeader()->setSectionResizeMode(EventTableModel::NameColumn, QHeaderView::Stretch);
        setCentralWidget(eventTableView_);

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

        openAction->setEnabled(true);
    }

} // namespace tracegraph::app
