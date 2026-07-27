#include "app/event_table_model.h"
#include "domain/trace_session.h"

namespace tracegraph::app
{

    EventTableModel::EventTableModel(QObject *parent)
        : QAbstractTableModel(parent) {}

    void EventTableModel::setSession(const domain::TraceSession *session)
    {
        beginResetModel();
        session_ = session;
        endResetModel();
    }

    int EventTableModel::rowCount(const QModelIndex &parent) const
    {
        if (parent.isValid() || session_ == nullptr)
        {
            return 0;
        }

        return static_cast<int>(session_->events().size());
    }

    int EventTableModel::columnCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
        {
            return 0;
        }

        return ColumnCount;
    }

    QVariant EventTableModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid() || session_ == nullptr)
        {
            return {};
        }

        const qsizetype row = static_cast<qsizetype>(index.row());

        if (row >= session_->events().size())
        {
            return {};
        }

        const domain::TraceEvent &event = session_->events().at(row);

        if (role == EventIdRole)
        {
            return event.id;
        }

        if (role != Qt::DisplayRole)
        {
            return {};
        }

        switch (index.column())
        {
        case IdColumn:
            return event.id;

        case NameColumn:
            return event.name;

        case CategoryColumn:
            return event.category;

        case ThreadColumn:
            return event.thread;

        case StartColumn:
            return event.startMicroseconds;

        case DurationColumn:
            return event.durationMicroseconds;

        default:
            return {};
        }
    }

    QVariant EventTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return {};
        }

        switch (section)
        {
        case IdColumn:
            return QStringLiteral("ID");

        case NameColumn:
            return QStringLiteral("Name");

        case CategoryColumn:
            return QStringLiteral("Category");

        case ThreadColumn:
            return QStringLiteral("Thread");

        case StartColumn:
            return QStringLiteral("Start (µs)");

        case DurationColumn:
            return QStringLiteral("Duration (µs)");

        default:
            return {};
        }
    }

} // namespace tracegraph::app
