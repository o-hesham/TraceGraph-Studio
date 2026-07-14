#pragma once

#include <optional>

#include <QtGlobal>
#include <QString>
#include <QVector>

namespace tracegraph::domain
{

    using EventId = qint64;

    struct TraceEvent
    {
        EventId id = 0;
        QString name;
        QString category;
        QString thread;
        qint64 startMicroseconds = 0;
        qint64 durationMicroseconds = 0;
        std::optional<EventId> parentId;
        QVector<EventId> dependencies;
    };

} // namespace tracegraph::domain
