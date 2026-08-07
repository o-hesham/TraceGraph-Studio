#pragma once

#include "trace_event.h"

#include <QVector>
#include <QHash>

namespace tracegraph::domain
{

    struct TraceSummary
    {
        qsizetype eventCount = 0;
        qsizetype threadCount = 0;
        qsizetype categoryCount = 0;
        quint64 durationMicroseconds = 0;
    };

    class TraceSession
    {
    public:
        explicit TraceSession(QVector<TraceEvent> events);
        [[nodiscard]] const QVector<TraceEvent> &events() const;
        [[nodiscard]] const TraceSummary &summary() const;
        [[nodiscard]] const TraceEvent *eventById(EventId id) const;
        [[nodiscard]] const QVector<EventId> &childIds(EventId parentId) const;
        [[nodiscard]] const QVector<EventId> &dependentIds(EventId dependencyId) const;

    private:
        QVector<TraceEvent> events_;
        TraceSummary summary_;
        QHash<EventId, qsizetype> eventIndexById_;

        QHash<EventId, QVector<EventId>> childIdsByParentId_;
        QHash<EventId, QVector<EventId>> dependentIdsByDependencyId_;
    };

} // namespace tracegraph::domain
