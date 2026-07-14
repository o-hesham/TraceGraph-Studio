#include "trace_session.h"

#include <utility>

namespace tracegraph::domain
{

    TraceSession::TraceSession(QVector<TraceEvent> events)
        : events_(std::move(events))
    {
        eventIndexById_.reserve(events_.size());
        for (qsizetype index = 0; index < events_.size(); index++)
        {
            // Map the event's ID to its position in the events vector for fast lookup.
            eventIndexById_.insert(events_[index].id, index);
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

} // namespace tracegraph::domain