#include "app/load_controller.h"
#include "io/trace_reader.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <QtConcurrentRun>

#include <utility>
#include <variant>

namespace tracegraph::app
{

    namespace
    {

        // Formats structured load errors for display to the user.
        [[nodiscard]] QString formatErrors(const domain::TraceLoadErrors &errors)
        {
            QStringList errorsList;
            for (const auto &error : errors)
            {
                errorsList.append(QStringLiteral("%1: %2").arg(error.location, error.message));
            }

            return errorsList.join(QStringLiteral("\n"));
        }

    } // namespace

    LoadController::LoadController(QWidget *dialogParent, QObject *parent)
        : QObject(parent), dialogParent_(dialogParent)
    {
        loadWatcher_ = new QFutureWatcher<domain::TraceLoadResult>(this);

        connect(loadWatcher_, &QFutureWatcher<domain::TraceLoadResult>::finished, this, &LoadController::handleLoadFinished);
    }

    const domain::TraceSession *LoadController::session() const
    {
        return session_ ? &session_.value() : nullptr;
    }

    void LoadController::openTraceFile()
    {
        if (loadWatcher_->isRunning())
        {
            return;
        }

        const QString filePath = QFileDialog::getOpenFileName(dialogParent_, QStringLiteral("Open Trace"), QString(), QStringLiteral("TraceGraph traces (*.tgtrace);;All files (*)"));

        if (filePath.isEmpty())
        {
            return;
        }

        emit loadingChanged(true);

        loadWatcher_->setFuture(QtConcurrent::run(
            [filePath]()
            {
                io::TraceReader traceReader;
                return traceReader.readFile(filePath);
            }));
    }

    void LoadController::handleLoadFinished()
    {
        auto loadResult = loadWatcher_->result();

        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&loadResult))
        {
            QMessageBox::critical(dialogParent_, QStringLiteral("Unable to Open Trace"), formatErrors(*errors));

            emit loadingChanged(false);

            return;
        }

        session_.emplace(std::get<domain::TraceSession>(std::move(loadResult)));

        emit sessionLoaded(&session_.value());
        emit loadingChanged(false);
    }

} // namespace tracegraph::app
