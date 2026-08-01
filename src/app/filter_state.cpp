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

} // namespace tracegraph::app
