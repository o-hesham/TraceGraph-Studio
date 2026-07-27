#include "app/selection_controller.h"

namespace tracegraph::app
{

    SelectionController::SelectionController(QObject *parent)
        : QObject(parent) {}

    void SelectionController::selectEvent(domain::EventId eventId)
    {
        if (selectedEventId_.has_value() && *selectedEventId_ == eventId)
        {
            return;
        }

        selectedEventId_ = eventId;
        emit selectedEventIdChanged(selectedEventId_);
    }

    void SelectionController::clearSelection()
    {
        if (!selectedEventId_.has_value())
        {
            return;
        }

        selectedEventId_.reset();
        emit selectedEventIdChanged(selectedEventId_);
    }

} // namespace tracegraph::app
