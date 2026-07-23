#pragma once

#include <QSortFilterProxyModel>

namespace tracegraph::app
{

    class EventFilterProxyModel final : public QSortFilterProxyModel
    {
    public:
        explicit EventFilterProxyModel(QObject *parent = nullptr);
    };

} // namespace tracegraph::app
