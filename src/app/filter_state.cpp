#include "app/filter_state.h"

namespace tracegraph::app
{

    FilterState::FilterState(QObject *parent)
        : QObject(parent) {}

    const QString &FilterState::filterText() const
    {
        return filterText_;
    }

    void FilterState::setFilterText(const QString &filterText)
    {
        if (filterText_ == filterText)
        {
            return;
        }

        filterText_ = filterText;
        emit filterTextChanged(filterText_);
    }

    qint64 FilterState::minimumDurationMicroseconds() const
    {
        return minimumDurationMicroseconds_;
    }

    void FilterState::setMinimumDurationMicroseconds(qint64 minimumDurationMicroseconds)
    {
        // Prevent -ve values.
        const qint64 boundedDuration = qMax<qint64>(0, minimumDurationMicroseconds);

        if (minimumDurationMicroseconds_ == boundedDuration)
        {
            return;
        }

        minimumDurationMicroseconds_ = boundedDuration;

        emit minimumDurationMicrosecondsChanged(minimumDurationMicroseconds_);
    }

    const QString &FilterState::threadFilter() const
    {
        return threadFilter_;
    }

    void FilterState::setThreadFilter(const QString &threadFilter)
    {
        if (threadFilter_ == threadFilter)
        {
            return;
        }

        threadFilter_ = threadFilter;
        emit threadFilterChanged(threadFilter_);
    }

    const QString &FilterState::categoryFilter() const
    {
        return categoryFilter_;
    }

    void FilterState::setCategoryFilter(
        const QString &categoryFilter)
    {
        if (categoryFilter_ == categoryFilter)
        {
            return;
        }

        categoryFilter_ = categoryFilter;
        emit categoryFilterChanged(categoryFilter_);
    }

} // namespace tracegraph::app
