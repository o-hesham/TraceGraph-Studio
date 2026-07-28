#include "app/event_inspector_widget.h"
#include "domain/trace_session.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QStringList>

namespace
{

    constexpr int MinimumInspectorWidth = 280;

    // Creates a value label whose contents can be selected and copied.
    [[nodiscard]] QLabel *createValueLabel(QWidget *parent)
    {
        auto *label = new QLabel(parent);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setWordWrap(true);

        return label;
    }

} // namespace

namespace tracegraph::app
{

    EventInspectorWidget::EventInspectorWidget(QWidget *parent)
        : QWidget(parent)
    {
        setMinimumWidth(MinimumInspectorWidth);

        auto *mainLayout = new QVBoxLayout(this);

        messageLabel_ = new QLabel(QStringLiteral("Select an event to inspect."), this);
        messageLabel_->setAlignment(Qt::AlignCenter);
        messageLabel_->setWordWrap(true);

        detailsWidget_ = new QWidget(this);
        auto *detailsLayout = new QFormLayout(detailsWidget_);

        idValueLabel_ = createValueLabel(detailsWidget_);
        nameValueLabel_ = createValueLabel(detailsWidget_);
        categoryValueLabel_ = createValueLabel(detailsWidget_);
        threadValueLabel_ = createValueLabel(detailsWidget_);
        startValueLabel_ = createValueLabel(detailsWidget_);
        durationValueLabel_ = createValueLabel(detailsWidget_);
        parentValueLabel_ = createValueLabel(detailsWidget_);
        dependenciesValueLabel_ = createValueLabel(detailsWidget_);

        detailsLayout->addRow(QStringLiteral("ID:"), idValueLabel_);
        detailsLayout->addRow(QStringLiteral("Name:"), nameValueLabel_);
        detailsLayout->addRow(QStringLiteral("Category:"), categoryValueLabel_);
        detailsLayout->addRow(QStringLiteral("Thread:"), threadValueLabel_);
        detailsLayout->addRow(QStringLiteral("Start:"), startValueLabel_);
        detailsLayout->addRow(QStringLiteral("Duration:"), durationValueLabel_);
        detailsLayout->addRow(QStringLiteral("Parent:"), parentValueLabel_);
        detailsLayout->addRow(QStringLiteral("Dependencies:"), dependenciesValueLabel_);

        mainLayout->addWidget(messageLabel_);
        mainLayout->addWidget(detailsWidget_);
        mainLayout->addStretch();

        refresh();
    }

    void EventInspectorWidget::setSession(const domain::TraceSession *session)
    {
        session_ = session;
        selectedEventId_.reset();
        refresh();
    }

    void EventInspectorWidget::setSelectedEventId(std::optional<domain::EventId> selectedEventId)
    {
        if (selectedEventId_ == selectedEventId)
        {
            return;
        }

        selectedEventId_ = selectedEventId;
        refresh();
    }

    void EventInspectorWidget::refresh()
    {
        if (session_ == nullptr)
        {
            detailsWidget_->hide();
            messageLabel_->setText(QStringLiteral("No trace loaded."));
            messageLabel_->show();

            return;
        }

        if (!selectedEventId_.has_value())
        {
            detailsWidget_->hide();
            messageLabel_->setText(QStringLiteral("Select an event to inspect."));
            messageLabel_->show();

            return;
        }

        const domain::TraceEvent *selectedEvent = session_->eventById(*selectedEventId_);
        if (selectedEvent == nullptr)
        {
            detailsWidget_->hide();
            messageLabel_->setText(QStringLiteral("Selected event is unavailable."));
            messageLabel_->show();

            return;
        }

        idValueLabel_->setText(QString::number(selectedEvent->id));
        nameValueLabel_->setText(selectedEvent->name);
        categoryValueLabel_->setText(selectedEvent->category);
        threadValueLabel_->setText(selectedEvent->thread);
        startValueLabel_->setText(QStringLiteral("%1 \u00B5s").arg(selectedEvent->startMicroseconds));
        durationValueLabel_->setText(QStringLiteral("%1 \u00B5s").arg(selectedEvent->durationMicroseconds));
        parentValueLabel_->setText(selectedEvent->parentId.has_value() ? QString::number(*selectedEvent->parentId) : QStringLiteral("None"));

        QStringList dependencyIds;
        dependencyIds.reserve(selectedEvent->dependencies.size());

        for (const auto &dependencyId : selectedEvent->dependencies)
        {
            dependencyIds.append(QString::number(dependencyId));
        }

        dependenciesValueLabel_->setText(dependencyIds.isEmpty() ? QStringLiteral("None") : dependencyIds.join(QStringLiteral(", ")));

        messageLabel_->hide();
        detailsWidget_->show();
    }

} // namespace tracegraph::app
