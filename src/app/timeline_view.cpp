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
    constexpr qreal MinimumZoomFactor = 1.0;
    constexpr qreal MaximumZoomFactor = 64.0;
    constexpr qreal ZoomStep = 1.25;

    // Converts a trace time into an x pixel position on the timeline.
    [[nodiscard]] qreal traceTimeToX(quint64 timestamp, quint64 timelineStart, quint64 timelineEnd, qreal plotLeft, qreal plotWidth)
    {
        if (timelineEnd <= timelineStart)
        {
            return plotLeft;
        }

        const long double position = static_cast<long double>(timestamp - timelineStart) / static_cast<long double>(timelineEnd - timelineStart);

        return plotLeft + static_cast<qreal>(position * plotWidth);
    }

    // Converts an x pixel position on the timeline into a time.
    [[nodiscard]] quint64 contentXToTraceTime(qreal contentX, qreal contentWidth, quint64 timelineStart, quint64 timelineEnd)
    {
        if (contentWidth <= 0.0 || timelineEnd <= timelineStart)
        {
            return timelineStart;
        }

        const qreal boundedX = qBound(qreal{0.0}, contentX, contentWidth);

        const long double position = static_cast<long double>(boundedX) / static_cast<long double>(contentWidth);

        return timelineStart + static_cast<quint64>(position * (timelineEnd - timelineStart));
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

        zoomFactor_ = 1.0;
        horizontalScrollBar()->setValue(0);

        rebuildTimelineMetadata();
        updateVerticalScrollBar();
        updateHorizontalScrollBar();
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

        const int visiblePlotWidth = qMax(1, plotRight - plotLeft);

        const qreal contentPlotWidth = visiblePlotWidth * zoomFactor_;

        const int horizontalOffset = horizontalScrollBar()->value();

        const qreal contentOriginX = plotLeft - horizontalOffset;

        const quint64 visibleStartTime = contentXToTraceTime(horizontalOffset, contentPlotWidth, timelineStartMicroseconds_, timelineEndMicroseconds_);

        const quint64 visibleEndTime = contentXToTraceTime(horizontalOffset + visiblePlotWidth, contentPlotWidth, timelineStartMicroseconds_, timelineEndMicroseconds_);

        // Draw the timeline header.
        painter.fillRect(QRect(0, 0, viewportRect.width(), HeaderHeight), palette().alternateBase());

        painter.setPen(palette().text().color());

        painter.drawText(QRect(Padding, 0, ThreadLabelWidth - Padding, HeaderHeight), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Thread"));

        painter.drawText(QRect(plotLeft, 0, visiblePlotWidth / 2, HeaderHeight), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("%1 µs").arg(visibleStartTime));

        painter.drawText(QRect(plotLeft + visiblePlotWidth / 2, 0, visiblePlotWidth - visiblePlotWidth / 2, HeaderHeight), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("%1 µs").arg(visibleEndTime));

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

        painter.setClipRect(QRectF(plotLeft, HeaderHeight, visiblePlotWidth, threadCount * LaneHeight));

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

            const qreal eventLeft = traceTimeToX(eventStart, timelineStartMicroseconds_, timelineEndMicroseconds_, contentOriginX, contentPlotWidth);

            const qreal eventRight = traceTimeToX(eventEnd, timelineStartMicroseconds_, timelineEndMicroseconds_, contentOriginX, contentPlotWidth);

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
        updateHorizontalScrollBar();
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

    void TimelineView::updateHorizontalScrollBar()
    {
        const int plotLeft = ThreadLabelWidth + Padding;

        const int plotRight = qMax(plotLeft, viewport()->rect().right() - Padding);

        const int visiblePlotWidth = qMax(1, plotRight - plotLeft);

        const int contentPlotWidth = qRound(visiblePlotWidth * zoomFactor_);

        QScrollBar *scrollBar = horizontalScrollBar();

        scrollBar->setRange(0, qMax(0, contentPlotWidth - visiblePlotWidth));

        scrollBar->setPageStep(visiblePlotWidth);

        scrollBar->setSingleStep(qMax(1, visiblePlotWidth / 10));
    }

    void TimelineView::wheelEvent(QWheelEvent *event)
    {
        if (!event->modifiers().testFlag(Qt::ControlModifier) || session_ == nullptr || session_->events().isEmpty())
        {
            QAbstractScrollArea::wheelEvent(event);
            return;
        }

        const int wheelDelta = event->angleDelta().y();

        if (wheelDelta == 0)
        {
            event->ignore();
            return;
        }

        const int plotLeft = ThreadLabelWidth + Padding;

        const int plotRight = qMax(plotLeft, viewport()->rect().right() - Padding);

        const int visiblePlotWidth = qMax(1, plotRight - plotLeft);

        const qreal cursorInPlot = qBound(qreal{0.0}, event->position().x() - plotLeft, static_cast<qreal>(visiblePlotWidth));

        const qreal oldContentPlotWidth = visiblePlotWidth * zoomFactor_;

        const qreal contentXUnderCursor = horizontalScrollBar()->value() + cursorInPlot;

        const qreal anchorPosition = contentXUnderCursor / oldContentPlotWidth;

        const qreal wheelSteps = static_cast<qreal>(wheelDelta) / 120.0;

        const qreal requestedZoom = zoomFactor_ * std::pow(ZoomStep, wheelSteps);

        const qreal newZoom = qBound(MinimumZoomFactor, requestedZoom, MaximumZoomFactor);

        if (qFuzzyCompare(newZoom, zoomFactor_))
        {
            event->accept();
            return;
        }

        zoomFactor_ = newZoom;
        updateHorizontalScrollBar();

        const qreal newContentPlotWidth = visiblePlotWidth * zoomFactor_;

        const int newScrollPosition = qRound(anchorPosition * newContentPlotWidth - cursorInPlot);

        horizontalScrollBar()->setValue(newScrollPosition);

        viewport()->update();
        event->accept();
    }

} // namespace tracegraph::app
