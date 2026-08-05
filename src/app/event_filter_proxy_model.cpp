#include "app/event_filter_proxy_model.h"
#include "app/event_table_model.h"

namespace tracegraph::app
{

    EventFilterProxyModel::EventFilterProxyModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
        setSortCaseSensitivity(Qt::CaseInsensitive);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setFilterKeyColumn(-1);
    }

    void EventFilterProxyModel::setMinimumDurationMicroseconds(qint64 minimumDurationMicroseconds)
    {
        // Prevent -ve values.
        const qint64 boundedDuration = qMax<qint64>(0, minimumDurationMicroseconds);

        if (minimumDurationMicroseconds_ == boundedDuration)
        {
            return;
        }

        beginFilterChange();
        minimumDurationMicroseconds_ = boundedDuration;
        endFilterChange(Direction::Rows);
    }

    void EventFilterProxyModel::setThreadFilter(const QString &threadFilter)
    {
        if (threadFilter_ == threadFilter)
        {
            return;
        }

        beginFilterChange();
        threadFilter_ = threadFilter;
        endFilterChange(Direction::Rows);
    }

    void EventFilterProxyModel::setCategoryFilter(
        const QString &categoryFilter)
    {
        if (categoryFilter_ == categoryFilter)
        {
            return;
        }

        beginFilterChange();
        categoryFilter_ = categoryFilter;
        endFilterChange(Direction::Rows);
    }

    bool EventFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!threadFilter_.isEmpty())
        {
            const QModelIndex threadIndex = sourceModel()->index(sourceRow, EventTableModel::ThreadColumn, sourceParent);
            if (threadIndex.data(Qt::DisplayRole).toString() != threadFilter_)
            {
                return false;
            }
        }

        const QModelIndex durationIndex = sourceModel()->index(sourceRow, EventTableModel::DurationColumn, sourceParent);
        if (durationIndex.data(Qt::DisplayRole).toLongLong() < minimumDurationMicroseconds_)
        {
            return false;
        }

        if (!categoryFilter_.isEmpty())
        {
            const QModelIndex categoryIndex = sourceModel()->index(sourceRow, EventTableModel::CategoryColumn, sourceParent);
            if (categoryIndex.data(Qt::DisplayRole).toString() != categoryFilter_)
            {
                return false;
            }
        }

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

} // namespace tracegraph::app