#pragma once

#include "trace_event.h"

#include <QVector>
#include <QHash>

namespace tracegraph::domain
{

    class TraceSession
    {
    public:
        explicit TraceSession(QVector<TraceEvent> events);
        [[nodiscard]] const QVector<TraceEvent> &events() const;
        [[nodiscard]] const TraceEvent *eventById(EventId id) const;

    private:
        QVector<TraceEvent> events_;
        QHash<EventId, qsizetype> eventIndexById_;
    };

} // namespace tracegraph::domain
