#pragma once

#include <QSortFilterProxyModel>
#include <QtGlobal>
#include <QString>

namespace tracegraph::app
{

    class EventFilterProxyModel final : public QSortFilterProxyModel
    {
    public:
        explicit EventFilterProxyModel(QObject *parent = nullptr);
        void setMinimumDurationMicroseconds(qint64 minimumDurationMicroseconds);
        void setThreadFilter(const QString &threadFilter);
        void setCategoryFilter(const QString &categoryFilter);

    protected:
        // Accepts a row only when it satisfies every active filter.
        [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    private:
        qint64 minimumDurationMicroseconds_ = 0;
        QString threadFilter_;
        QString categoryFilter_;
    };

} // namespace tracegraph::app
