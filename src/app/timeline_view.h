#pragma once

#include <QAbstractScrollArea>
#include <QHash>
#include <QStringList>

class QResizeEvent;

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class TimelineView final : public QAbstractScrollArea
    {
    public:
        explicit TimelineView(QWidget *parent = nullptr);

        void setSession(const domain::TraceSession *session);

    protected:
        void paintEvent(QPaintEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;

    private:
        // Recomputes thread lanes and time bounds for a new session.
        void rebuildTimelineMetadata();

        // Updates vertical scrolling when the session or viewport size changes.
        void updateVerticalScrollBar();

        const domain::TraceSession *session_ = nullptr;

        QStringList threadNames_;
        QHash<QString, int> laneByThread_;

        quint64 timelineStartMicroseconds_ = 0;
        quint64 timelineEndMicroseconds_ = 0;
    };

} // namespace tracegraph::app