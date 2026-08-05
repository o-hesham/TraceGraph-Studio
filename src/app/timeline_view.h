#pragma once

#include "domain/trace_event.h"

#include <QAbstractScrollArea>
#include <QHash>
#include <QStringList>
#include <QVector>

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
        void setFilterText(const QString &filterText);
        void setMinimumDurationMicroseconds(qint64 minimumDurationMicroseconds);
        void setThreadFilter(const QString &threadFilter);
        void setCategoryFilter(const QString &categoryFilter);

    signals:
        void eventSelected(domain::EventId eventId);
        void selectionCleared();

    protected:
        void paintEvent(QPaintEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

    private:
        struct EventLayout
        {
            qsizetype eventIndex = 0;
            int lane = 0;
            qreal normalizedStart = 0.0;
            qreal normalizedEnd = 0.0;
        };

        // Recomputes thread lanes and time bounds for a new session.
        void rebuildTimelineMetadata();

        // Updates vertical scrolling when the session or viewport size changes.
        void updateVerticalScrollBar();
        // Updates horizontal  scrolling for the current zoom level.
        void updateHorizontalScrollBar();

        // Calculates an event rectangle using its cached layout.
        [[nodiscard]] QRectF eventRectangle(const EventLayout &layout, qreal contentOriginX, qreal contentPlotWidth, int verticalOffset) const;

        // Which timeline event is underneath this mouse position.
        [[nodiscard]] const domain::TraceEvent *eventAtPosition(const QPointF &position) const;

        // Checks whether an event matches the current text filter.
        [[nodiscard]] bool eventMatchesFilter(const domain::TraceEvent &event) const;

        const domain::TraceSession *session_ = nullptr;

        QStringList threadNames_;
        QHash<QString, int> laneByThread_;
        QVector<EventLayout> eventLayouts_;

        quint64 timelineStartMicroseconds_ = 0;
        quint64 timelineEndMicroseconds_ = 0;

        qint64 minimumDurationMicroseconds_ = 0;

        qreal zoomFactor_ = 1.0;

        QString filterText_;
        QString threadFilter_;
        QString categoryFilter_;

        std::optional<domain::EventId> selectedEventId_;
    };

} // namespace tracegraph::app