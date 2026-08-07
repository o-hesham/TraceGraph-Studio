#pragma once

#include <QWidget>

class QLabel;

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class TraceSummaryWidget final : public QWidget
    {
    public:
        explicit TraceSummaryWidget(QWidget *parent = nullptr);

        void setSession(const domain::TraceSession *session);

    private:
        // Refreshes the displayed statistics from the current session.
        void refresh();

        const domain::TraceSession *session_ = nullptr;

        QLabel *messageLabel_ = nullptr;
        QWidget *detailsWidget_ = nullptr;

        QLabel *eventCountValueLabel_ = nullptr;
        QLabel *threadCountValueLabel_ = nullptr;
        QLabel *categoryCountValueLabel_ = nullptr;
        QLabel *durationValueLabel_ = nullptr;
    };

} // namespace tracegraph::app
