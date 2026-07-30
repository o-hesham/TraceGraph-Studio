#include "app/dependency_graph_view.h"
#include "domain/trace_session.h"

#include <cmath>

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QBrush>
#include <QGraphicsRectItem>
#include <QPen>
#include <QPalette>
#include <QPointF>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QLineF>
#include <QPolygonF>
#include <QVector>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QVariant>
#include <QWheelEvent>

namespace
{

    constexpr qreal ScenePadding = 40.0;
    constexpr int MinimumGraphHeight = 220;
    constexpr qreal NodeWidth = 220.0;
    constexpr qreal NodeHeight = 90.0;
    constexpr qreal NodePadding = 10.0;
    constexpr qreal HorizontalNodeGap = 140.0;
    constexpr qreal VerticalNodeGap = 40.0;
    constexpr qreal ArrowLength = 14.0;
    constexpr qreal ArrowHalfWidth = 6.0;
    constexpr int EventIdDataKey = 0;
    constexpr qreal MinimumGraphScale = 0.25;
    constexpr qreal MaximumGraphScale = 4.0;
    constexpr qreal GraphZoomStep = 1.15;

    // Adds one event node to the scene at the given center position.
    void addEventNode(QGraphicsScene *scene, const tracegraph::domain::TraceEvent &event, const QPalette &palette, const QPointF &position, bool isSelected)
    {
        const QRectF nodeRect(-NodeWidth / 2.0, -NodeHeight / 2.0, NodeWidth, NodeHeight);

        const QColor fillColor = isSelected ? palette.highlight().color() : palette.button().color();
        const QColor textColor = isSelected ? palette.highlightedText().color() : palette.buttonText().color();

        QPen outlinePen(isSelected ? palette.highlightedText().color() : palette.mid().color());
        outlinePen.setWidthF(isSelected ? 2.0 : 1.0);

        QGraphicsRectItem *nodeItem = scene->addRect(nodeRect, outlinePen, QBrush(fillColor));
        nodeItem->setData(EventIdDataKey, QVariant::fromValue(event.id));
        nodeItem->setCursor(Qt::PointingHandCursor);
        nodeItem->setPos(position);
        nodeItem->setToolTip(QStringLiteral("Event ID: %1").arg(event.id));

        QGraphicsTextItem *textItem = new QGraphicsTextItem(nodeItem);
        textItem->setPlainText(
            QStringLiteral("%1\n%2 / %3\n%4 \u00B5s")
                .arg(event.name)
                .arg(event.category)
                .arg(event.thread)
                .arg(event.durationMicroseconds));
        textItem->setDefaultTextColor(textColor);
        textItem->setTextWidth(NodeWidth - 2.0 * NodePadding);

        const QRectF textBounds = textItem->boundingRect();

        textItem->setPos(-NodeWidth / 2.0 + NodePadding, -textBounds.height() / 2.0);
    }

    // Draws a directed relationship edge between two graph nodes.
    void addDirectedEdge(QGraphicsScene *scene, const QPointF &start, const QPointF &end, const QPalette &palette, Qt::PenStyle lineStyle, const QString &tooltip)
    {
        const QLineF edgeLine(start, end);
        if (edgeLine.length() <= 0.0)
        {
            return;
        }

        QColor edgeColor = palette.text().color();
        edgeColor.setAlpha(180);

        QPen edgePen(edgeColor, 1.5);
        edgePen.setStyle(lineStyle);

        QGraphicsLineItem *lineItem = scene->addLine(edgeLine, edgePen);
        lineItem->setZValue(-1.0);
        lineItem->setToolTip(tooltip);

        const QLineF unitLine = edgeLine.unitVector();
        const QPointF direction = unitLine.p2() - unitLine.p1();

        const QPointF perpendicular(-direction.y(), direction.x());

        const QPointF arrowBase = end - direction * ArrowLength;

        QPolygonF arrowHead;
        arrowHead << end << arrowBase + perpendicular * ArrowHalfWidth
                  << arrowBase - perpendicular * ArrowHalfWidth;

        QPen arrowPen(edgeColor, 1.5);

        QGraphicsPolygonItem *arrowItem = scene->addPolygon(arrowHead, arrowPen, QBrush(edgeColor));

        arrowItem->setToolTip(tooltip);
        arrowItem->setZValue(-1.0);
    }

} // namespace

namespace tracegraph::app
{

    DependencyGraphView::DependencyGraphView(QWidget *parent)
        : QGraphicsView(parent), scene_(new QGraphicsScene(this))
    {
        setMinimumHeight(MinimumGraphHeight);
        setScene(scene_);
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
        rebuildGraph();
    }

    void DependencyGraphView::setSession(const domain::TraceSession *session)
    {
        session_ = session;
        selectedEventId_.reset();
        resetTransform();
        rebuildGraph();
    }

    void DependencyGraphView::setSelectedEventId(std::optional<domain::EventId> selectedEventId)
    {
        if (selectedEventId_ == selectedEventId)
        {
            return;
        }

        selectedEventId_ = selectedEventId;
        rebuildGraph();
    }

    void DependencyGraphView::mousePressEvent(QMouseEvent *event)
    {
        if (event->button() != Qt::LeftButton)
        {
            QGraphicsView::mousePressEvent(event);
            return;
        }

        QGraphicsItem *clickedItem = itemAt(event->position().toPoint());

        while (clickedItem != nullptr)
        {
            const QVariant eventIdData = clickedItem->data(EventIdDataKey);
            if (eventIdData.isValid())
            {
                emit eventSelected(eventIdData.toLongLong());

                event->accept();
                return;
            }

            clickedItem = clickedItem->parentItem();
        }

        QGraphicsView::mousePressEvent(event);
    }

    void DependencyGraphView::wheelEvent(QWheelEvent *event)
    {
        if (!(event->modifiers() & Qt::ControlModifier) || event->angleDelta().y() == 0)
        {
            QGraphicsView::wheelEvent(event);
            return;
        }

        const qreal zoomSteps = event->angleDelta().y() / 120.0;

        const qreal requestedFactor = std::pow(GraphZoomStep, zoomSteps);

        const qreal currentScale = transform().m11();

        const qreal targetScale = qBound(MinimumGraphScale, currentScale * requestedFactor, MaximumGraphScale);

        const qreal appliedFactor = targetScale / currentScale;

        scale(appliedFactor, appliedFactor);
        event->accept();
    }

    void DependencyGraphView::rebuildGraph()
    {
        scene_->clear();

        QString message;

        if (session_ == nullptr)
        {
            message = QStringLiteral("No trace loaded.");
        }
        else if (!selectedEventId_.has_value())
        {
            message = QStringLiteral("Select an event to view dependencies.");
        }
        else
        {
            const auto *selectedEvent = session_->eventById(*selectedEventId_);

            QVector<const domain::TraceEvent *> dependencyEvents;
            dependencyEvents.reserve(selectedEvent->dependencies.size());

            for (const auto &dependencyId : selectedEvent->dependencies)
            {
                const domain::TraceEvent *dependencyEvent = session_->eventById(dependencyId);
                if (dependencyEvent != nullptr)
                {
                    dependencyEvents.append(dependencyEvent);
                }
            }

            const int dependencyCount = static_cast<int>(dependencyEvents.size());

            const qreal selectedX = dependencyEvents.isEmpty() ? 0.0 : NodeWidth + HorizontalNodeGap;

            const QPointF selectedPosition(selectedX, 0.0);

            const qreal dependencySpacing = NodeHeight + VerticalNodeGap;

            const qreal firstDependencyY = -0.5 * (dependencyCount - 1) * dependencySpacing;

            for (int index = 0; index < dependencyCount; ++index)
            {
                const QPointF dependencyPosition(0.0, firstDependencyY + index * dependencySpacing);

                addDirectedEdge(scene_, dependencyPosition + QPointF(NodeWidth / 2.0, 0.0), selectedPosition - QPointF(NodeWidth / 2.0, 0.0), palette(), Qt::SolidLine, QStringLiteral("Dependency relationship"));

                addEventNode(scene_, *dependencyEvents.at(index), palette(), dependencyPosition, false);
            }

            if (selectedEvent->parentId.has_value())
            {
                const domain::TraceEvent *parentEvent = session_->eventById(*selectedEvent->parentId);

                if (parentEvent != nullptr)
                {
                    const QPointF parentPosition(selectedPosition.x(), selectedPosition.y() - NodeHeight - VerticalNodeGap);

                    addDirectedEdge(scene_, parentPosition + QPointF(0.0, NodeHeight / 2.0), selectedPosition - QPointF(0.0, NodeHeight / 2.0), palette(), Qt::DashLine, QStringLiteral("Parent relationship"));

                    addEventNode(scene_, *parentEvent, palette(), parentPosition, false);
                }
            }

            addEventNode(scene_, *selectedEvent, palette(), selectedPosition, true);

            scene_->setSceneRect(scene_->itemsBoundingRect().adjusted(-ScenePadding, -ScenePadding, ScenePadding, ScenePadding));

            centerOn(scene_->sceneRect().center());

            return;
        }

        QGraphicsTextItem *messageItem = scene_->addText(message);
        messageItem->setDefaultTextColor(palette().text().color());

        scene_->setSceneRect(messageItem->boundingRect().adjusted(-ScenePadding, -ScenePadding, ScenePadding, ScenePadding));
    }

} // namespace tracegraph::app
