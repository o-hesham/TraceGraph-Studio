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
        [[nodiscard]] const QVector<EventId> &childIds(EventId parentId) const;
        [[nodiscard]] const QVector<EventId> &dependentIds(EventId dependencyId) const;

    private:
        QVector<TraceEvent> events_;
        QHash<EventId, qsizetype> eventIndexById_;

        QHash<EventId, QVector<EventId>> childIdsByParentId_;
        QHash<EventId, QVector<EventId>> dependentIdsByDependencyId_;
    };

} // namespace tracegraph::domain
