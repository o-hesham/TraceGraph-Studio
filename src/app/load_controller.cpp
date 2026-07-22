#include "app/load_controller.h"
#include "io/trace_reader.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>

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
        : QObject(parent), dialogParent_(dialogParent) {}

    const domain::TraceSession *LoadController::session() const
    {
        return session_ ? &session_.value() : nullptr;
    }

    void LoadController::openTraceFile()
    {
        const QString filePath = QFileDialog::getOpenFileName(dialogParent_, QStringLiteral("Open Trace"), QString(), QStringLiteral("TraceGraph traces (*.tgtrace);;All files (*)"));

        if (filePath.isEmpty())
            return;

        io::TraceReader traceReader;
        auto loadResult = traceReader.readFile(filePath);

        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&loadResult))
        {
            QMessageBox::critical(
                dialogParent_,
                QStringLiteral("Unable to Open Trace"),
                formatErrors(*errors));

            return;
        }

        session_.emplace(std::get<domain::TraceSession>(std::move(loadResult)));

        emit sessionLoaded(&session_.value());
    }

} // namespace tracegraph::app
