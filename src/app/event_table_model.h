#pragma once

#include <QAbstractTableModel>

namespace tracegraph::domain
{
    class TraceSession;
} // namespace tracegraph::domain

namespace tracegraph::app
{

    class EventTableModel final : public QAbstractTableModel
    {
    public:
        explicit EventTableModel(QObject *parent = nullptr);

        void setSession(const domain::TraceSession *session);

        [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    private:
        enum Column
        {
            IdColumn,
            NameColumn,
            CategoryColumn,
            ThreadColumn,
            StartColumn,
            DurationColumn,
            ColumnCount
        };

        const domain::TraceSession *session_ = nullptr;
    };

} // namespace tracegraph::app
