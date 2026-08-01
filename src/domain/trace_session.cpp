#include "trace_session.h"

#include <utility>

namespace tracegraph::domain
{

    TraceSession::TraceSession(QVector<TraceEvent> events)
        : events_(std::move(events))
    {
        eventIndexById_.reserve(events_.size());
        childIdsByParentId_.reserve(events_.size());
        dependentIdsByDependencyId_.reserve(events_.size());
        for (qsizetype index = 0; index < events_.size(); index++)
        {
            const TraceEvent &event = events_[index];

            // Map the event's ID to its position in the events vector for fast lookup.
            eventIndexById_.insert(event.id, index);

            if (event.parentId.has_value())
            {
                childIdsByParentId_[*event.parentId].append(event.id);
            }

            for (EventId dependencyId : event.dependencies)
            {
                dependentIdsByDependencyId_[dependencyId].append(event.id);
            }
        }
    }

    const QVector<TraceEvent> &TraceSession::events() const
    {
        return events_;
    }

    // Return the event with the given ID, or nullptr if no matching event exists.
    const TraceEvent *TraceSession::eventById(EventId id) const
    {
        const auto result = eventIndexById_.constFind(id);
        if (result == eventIndexById_.constEnd())
        {
            return nullptr;
        }

        return &events_[result.value()];
    }

    const QVector<EventId> &TraceSession::childIds(EventId parentId) const
    {
        static const QVector<EventId> emptyIds;

        const auto result = childIdsByParentId_.constFind(parentId);

        return result == childIdsByParentId_.constEnd() ? emptyIds : result.value();
    }

    const QVector<EventId> &TraceSession::dependentIds(EventId dependencyId) const
    {
        static const QVector<EventId> emptyIds;

        const auto result = dependentIdsByDependencyId_.constFind(dependencyId);

        return result == dependentIdsByDependencyId_.constEnd() ? emptyIds : result.value();
    }

} // namespace tracegraph::domain