#pragma once

#include "domain/trace_event.h"

#include <QObject>

#include <optional>

namespace tracegraph::app
{
    class SelectionController final : public QObject
    {
        Q_OBJECT

    public:
        explicit SelectionController(QObject *parent = nullptr);

    public slots:
        void selectEvent(domain::EventId eventId);
        void clearSelection();

    signals:
        void selectedEventIdChanged(std::optional<domain::EventId> selectedEventId);

    private:
        std::optional<domain::EventId> selectedEventId_;
    };

} // namespace tracegraph::app
