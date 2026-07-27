#pragma once

#include "domain/trace_event.h"

#include <QAbstractScrollArea>
#include <QHash>
#include <QStringList>

#include <optional>

class QResizeEvent;
class QWheelEvent;
class QMouseEvent;
class QRectF;
class QPointF;

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class TimelineView final : public QAbstractScrollArea
    {
        Q_OBJECT

    public:
        explicit TimelineView(QWidget *parent = nullptr);

        void setSession(const domain::TraceSession *session);

        // Updates the highlight when another widget selects an event.
        void setSelectedEventId(std::optional<domain::EventId> eventId);

    signals:
        void eventSelected(domain::EventId eventId);
        void selectionCleared();

    protected:
        void paintEvent(QPaintEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

    private:
        // Recomputes thread lanes and time bounds for a new session.
        void rebuildTimelineMetadata();

        // Updates vertical scrolling when the session or viewport size changes.
        void updateVerticalScrollBar();
        // Updates horizontal  scrolling for the current zoom level.
        void updateHorizontalScrollBar();

        // Calculates an event's rectangle after zooming and scrolling.
        [[nodiscard]] QRectF eventRectangle(const domain::TraceEvent &traceEvent, qreal contentOriginX, qreal contentPlotWidth, int verticalOffset) const;

        // Which timeline event is underneath this mouse position.
        [[nodiscard]] const domain::TraceEvent *eventAtPosition(const QPointF &position) const;

        const domain::TraceSession *session_ = nullptr;

        QStringList threadNames_;
        QHash<QString, int> laneByThread_;

        quint64 timelineStartMicroseconds_ = 0;
        quint64 timelineEndMicroseconds_ = 0;

        qreal zoomFactor_ = 1.0;

        std::optional<domain::EventId> selectedEventId_;
    };

} // namespace tracegraph::app