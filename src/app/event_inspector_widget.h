#pragma once

#include "domain/trace_event.h"

#include <QWidget>

#include <optional>

class QLabel;

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class EventInspectorWidget final : public QWidget
    {
    public:
        explicit EventInspectorWidget(QWidget *parent = nullptr);

        void setSession(const domain::TraceSession *session);
        void setSelectedEventId(std::optional<domain::EventId> selectedEventId);

    private:
        // Refreshes the displayed fields from the current session and selection.
        void refresh();

        const domain::TraceSession *session_ = nullptr;
        std::optional<domain::EventId> selectedEventId_;

        QLabel *messageLabel_ = nullptr;
        QWidget *detailsWidget_ = nullptr;

        QLabel *idValueLabel_ = nullptr;
        QLabel *nameValueLabel_ = nullptr;
        QLabel *categoryValueLabel_ = nullptr;
        QLabel *threadValueLabel_ = nullptr;
        QLabel *startValueLabel_ = nullptr;
        QLabel *durationValueLabel_ = nullptr;
        QLabel *parentValueLabel_ = nullptr;
        QLabel *dependenciesValueLabel_ = nullptr;
    };

} // namespace tracegraph::app
