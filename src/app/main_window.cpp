#include "app/main_window.h"
#include "app/load_controller.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>

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

                    sessionStatusLabel_->setText(QStringLiteral("Loaded %1 events.").arg(session->events().size()));
                });

        openAction->setEnabled(true);
    }

} // namespace tracegraph::app
