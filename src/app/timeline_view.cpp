#include "app/timeline_view.h"
#include "domain/trace_session.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScrollBar>

namespace
{

    constexpr int HeaderHeight = 32;
    constexpr int ThreadLabelWidth = 140;
    constexpr int LaneHeight = 42;
    constexpr int Padding = 8;
    constexpr int EventVerticalPadding = 6;
    constexpr qreal MinimumEventWidth = 3.0;

    // Maps an absolute trace timestamp into the plot's x-coordinate.
    [[nodiscard]] qreal traceTimeToX(quint64 timestamp, quint64 timelineStart, quint64 timelineEnd, qreal plotLeft, qreal plotWidth)
    {
        if (timelineEnd <= timelineStart)
        {
            return plotLeft;
        }

        const long double position = static_cast<long double>(timestamp - timelineStart) / static_cast<long double>(timelineEnd - timelineStart);

        return plotLeft + static_cast<qreal>(position * plotWidth);
    }

} // namespace

namespace tracegraph::app
{

    TimelineView::TimelineView(QWidget *parent)
        : QAbstractScrollArea(parent)
    {
        setMinimumHeight(200);
    }

    void TimelineView::setSession(const domain::TraceSession *session)
    {
        if (session_ == session)
        {
            return;
        }

        session_ = session;
        rebuildTimelineMetadata();
        updateVerticalScrollBar();
        viewport()->update();
    }

    void TimelineView::rebuildTimelineMetadata()
    {
        threadNames_.clear();
        laneByThread_.clear();

        timelineStartMicroseconds_ = 0;
        timelineEndMicroseconds_ = 0;

        if (session_ == nullptr || session_->events().isEmpty())
        {
            return;
        }

        bool firstEvent = true;

        for (const auto &event : session_->events())
        {
            if (!laneByThread_.contains(event.thread))
            {
                const int lane = static_cast<int>(threadNames_.size());

                laneByThread_.insert(event.thread, lane);
                threadNames_.append(event.thread);
            }

            const quint64 start = static_cast<quint64>(event.startMicroseconds);
            const quint64 end = start + static_cast<quint64>(event.durationMicroseconds);

            if (firstEvent)
            {
                timelineStartMicroseconds_ = start;
                timelineEndMicroseconds_ = end;
                firstEvent = false;
            }
            else
            {
                timelineStartMicroseconds_ = qMin(timelineStartMicroseconds_, start);
                timelineEndMicroseconds_ = qMax(timelineEndMicroseconds_, end);
            }
        }
    }

    void TimelineView::paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event);

        QPainter painter(viewport());

        painter.fillRect(viewport()->rect(), palette().base());

        painter.setPen(palette().text().color());

        if (session_ == nullptr)
        {
            painter.drawText(viewport()->rect(), Qt::AlignCenter, QStringLiteral("No trace loaded"));

            return;
        }

        if (session_->events().isEmpty())
        {
            painter.drawText(viewport()->rect(), Qt::AlignCenter, QStringLiteral("Trace contains no events"));
            return;
        }

        const QRect viewportRect = viewport()->rect();

        const int plotLeft = ThreadLabelWidth + Padding;

        const int plotRight = qMax(plotLeft, viewportRect.right() - Padding);

        const int plotWidth = qMax(1, plotRight - plotLeft);

        // Draw the timeline header.
        painter.fillRect(QRect(0, 0, viewportRect.width(), HeaderHeight), palette().alternateBase());

        painter.setPen(palette().text().color());

        painter.drawText(QRect(Padding, 0, ThreadLabelWidth - Padding, HeaderHeight), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Thread"));

        painter.drawText(QRect(plotLeft, 0, plotWidth / 2, HeaderHeight), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("%1 µs").arg(timelineStartMicroseconds_));

        painter.drawText(QRect(plotLeft + plotWidth / 2, 0, plotWidth - plotWidth / 2, HeaderHeight), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("%1 µs").arg(timelineEndMicroseconds_));

        // Draw one row for each thread.
        const int threadCount = static_cast<int>(threadNames_.size());

        const int verticalOffset = verticalScrollBar()->value();

        painter.save();

        painter.setClipRect(QRect(0, HeaderHeight, viewportRect.width(), qMax(0, viewportRect.height() - HeaderHeight)));

        for (int lane = 0; lane < threadCount; ++lane)
        {
            const int laneTop = HeaderHeight + lane * LaneHeight - verticalOffset;

            const QRect laneRect(0, laneTop, viewportRect.width(), LaneHeight);

            if (lane % 2 != 0)
            {
                painter.fillRect(laneRect, palette().alternateBase());
            }

            painter.setPen(palette().mid().color());

            painter.drawLine(0, laneRect.bottom(), viewportRect.right(), laneRect.bottom());

            painter.setPen(palette().text().color());

            painter.drawText(QRect(Padding, laneTop, ThreadLabelWidth - Padding, LaneHeight), Qt::AlignLeft | Qt::AlignVCenter, threadNames_.at(lane));
        }

        painter.restore();

        painter.save();

        painter.setClipRect(QRectF(plotLeft, HeaderHeight, plotWidth, threadCount * LaneHeight));

        for (const domain::TraceEvent &traceEvent :
             session_->events())
        {
            const auto laneIterator = laneByThread_.constFind(traceEvent.thread);

            if (laneIterator == laneByThread_.cend())
            {
                continue;
            }

            const int lane = laneIterator.value();

            const quint64 eventStart = static_cast<quint64>(traceEvent.startMicroseconds);

            const quint64 eventEnd = eventStart + static_cast<quint64>(traceEvent.durationMicroseconds);

            const qreal eventLeft = traceTimeToX(eventStart, timelineStartMicroseconds_, timelineEndMicroseconds_, plotLeft, plotWidth);

            const qreal eventRight = traceTimeToX(eventEnd, timelineStartMicroseconds_, timelineEndMicroseconds_, plotLeft, plotWidth);

            const qreal eventWidth = qMax(MinimumEventWidth, eventRight - eventLeft);

            const qreal eventTop = HeaderHeight + lane * LaneHeight + EventVerticalPadding - verticalOffset;
            const QRectF eventRect(eventLeft, eventTop, eventWidth, LaneHeight - 2 * EventVerticalPadding);

            QColor fillColor = palette().highlight().color();

            fillColor.setAlpha(200);

            painter.setPen(fillColor.lighter(120));
            painter.setBrush(fillColor);

            painter.drawRoundedRect(eventRect, 3.0, 3.0);

            const int requiredTextWidth = painter.fontMetrics().horizontalAdvance(traceEvent.name) + 2 * Padding;

            if (eventRect.width() >= requiredTextWidth)
            {
                painter.setPen(palette().highlightedText().color());

                painter.drawText(eventRect.adjusted(Padding, 0, -Padding, 0), Qt::AlignLeft | Qt::AlignVCenter, traceEvent.name);
            }
        }

        painter.restore();

        // Separate thread labels from the time area.
        painter.setPen(palette().mid().color());

        painter.drawLine(ThreadLabelWidth, 0, ThreadLabelWidth, viewportRect.bottom());
    }

    void TimelineView::resizeEvent(QResizeEvent *event)
    {
        QAbstractScrollArea::resizeEvent(event);
        updateVerticalScrollBar();
    }

    void TimelineView::updateVerticalScrollBar()
    {
        const int contentHeight = static_cast<int>(threadNames_.size()) * LaneHeight;

        const int visibleHeight = qMax(0, viewport()->height() - HeaderHeight);

        QScrollBar *scrollBar = verticalScrollBar();

        scrollBar->setRange(0, qMax(0, contentHeight - visibleHeight));

        scrollBar->setPageStep(visibleHeight);
        scrollBar->setSingleStep(LaneHeight);
    }

} // namespace tracegraph::app
