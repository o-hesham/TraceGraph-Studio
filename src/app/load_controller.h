#pragma once

#include "domain/trace_load_result.h"

#include <optional>

#include <QObject>
#include <QFutureWatcher>

class QWidget;

namespace tracegraph::app
{

    class LoadController final : public QObject
    {
        Q_OBJECT

    public:
        explicit LoadController(QWidget *dialogParent, QObject *parent = nullptr);

        [[nodiscard]] const domain::TraceSession *session() const;

    public slots:
        void openTraceFile();

    signals:
        void sessionLoaded(const domain::TraceSession *session);
        void loadingChanged(bool isLoading);

    private:
        // Processes a completed background load on the GUI thread.
        void handleLoadFinished();

        QWidget *dialogParent_ = nullptr;
        std::optional<domain::TraceSession> session_;

        QFutureWatcher<domain::TraceLoadResult> *loadWatcher_ = nullptr;
    };

} // namespace tracegraph::app