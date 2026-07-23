#include "app/event_filter_proxy_model.h"

namespace tracegraph::app
{

    EventFilterProxyModel::EventFilterProxyModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
        setSortCaseSensitivity(Qt::CaseInsensitive);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setFilterKeyColumn(-1);
    }

} // namespace tracegraph::app