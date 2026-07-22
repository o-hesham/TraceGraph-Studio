#pragma once

#include "domain/trace_session.h"

#include <QObject>

#include <optional>

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

    private:
        QWidget *dialogParent_ = nullptr;
        std::optional<domain::TraceSession> session_;
    };

} // namespace tracegraph::app