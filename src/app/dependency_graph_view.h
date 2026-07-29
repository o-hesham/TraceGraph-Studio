#pragma once

#include "domain/trace_event.h"

#include <QGraphicsView>

#include <optional>

class QGraphicsScene;

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class DependencyGraphView final : public QGraphicsView
    {
    public:
        explicit DependencyGraphView(QWidget *parent = nullptr);

        void setSession(const domain::TraceSession *session);
        void setSelectedEventId(std::optional<domain::EventId> selectedEventId);

    private:
        // Rebuilds the scene for the currently selected event.
        void rebuildGraph();

        QGraphicsScene *scene_ = nullptr;

        const domain::TraceSession *session_ = nullptr;
        std::optional<domain::EventId> selectedEventId_;
    };

} // namespace tracegraph::app
