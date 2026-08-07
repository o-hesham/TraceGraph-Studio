#include "app/trace_summary_widget.h"
#include "domain/trace_session.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace
{

    constexpr int MinimumSummaryWidth = 280;

    // Creates a value label whose text can be selected and copied.
    [[nodiscard]] QLabel *createValueLabel(QWidget *parent)
    {
        auto *label = new QLabel(parent);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);

        return label;
    }

} // namespace

namespace tracegraph::app
{

    TraceSummaryWidget::TraceSummaryWidget(QWidget *parent)
        : QWidget(parent)
    {
        setMinimumWidth(MinimumSummaryWidth);

        auto *mainLayout = new QVBoxLayout(this);

        messageLabel_ = new QLabel(QStringLiteral("No trace loaded."), this);
        messageLabel_->setAlignment(Qt::AlignCenter);
        messageLabel_->setWordWrap(true);

        detailsWidget_ = new QWidget(this);
        auto *detailsLayout = new QFormLayout(detailsWidget_);

        eventCountValueLabel_ = createValueLabel(detailsWidget_);
        threadCountValueLabel_ = createValueLabel(detailsWidget_);
        categoryCountValueLabel_ = createValueLabel(detailsWidget_);
        durationValueLabel_ = createValueLabel(detailsWidget_);

        detailsLayout->addRow(QStringLiteral("Events:"), eventCountValueLabel_);
        detailsLayout->addRow(QStringLiteral("Threads:"), threadCountValueLabel_);
        detailsLayout->addRow(QStringLiteral("Categories:"), categoryCountValueLabel_);
        detailsLayout->addRow(QStringLiteral("Trace duration:"), durationValueLabel_);

        mainLayout->addWidget(messageLabel_);
        mainLayout->addWidget(detailsWidget_);
        mainLayout->addStretch();

        detailsWidget_->hide();
    }

    void TraceSummaryWidget::setSession(const domain::TraceSession *session)
    {
        session_ = session;
        refresh();
    }

    void TraceSummaryWidget::refresh()
    {
        if (session_ == nullptr)
        {
            detailsWidget_->hide();
            messageLabel_->setText(QStringLiteral("No trace loaded."));
            messageLabel_->show();

            return;
        }

        const domain::TraceSummary &summary = session_->summary();

        eventCountValueLabel_->setText(QString::number(summary.eventCount));
        threadCountValueLabel_->setText(QString::number(summary.threadCount));
        categoryCountValueLabel_->setText(QString::number(summary.categoryCount));
        durationValueLabel_->setText(QStringLiteral("%1 \u00B5s").arg(summary.durationMicroseconds));

        messageLabel_->hide();
        detailsWidget_->show();
    }

} // namespace tracegraph::app
