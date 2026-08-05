#pragma once

#include <QObject>
#include <QString>
#include <QtGlobal>

namespace tracegraph::app
{

    class FilterState final : public QObject
    {
        Q_OBJECT

    public:
        explicit FilterState(QObject *parent = nullptr);

        [[nodiscard]] const QString &filterText() const;
        void setFilterText(const QString &filterText);
        [[nodiscard]] qint64 minimumDurationMicroseconds() const;
        void setMinimumDurationMicroseconds(qint64 minimumDurationMicroseconds);
        [[nodiscard]] const QString &threadFilter() const;
        void setThreadFilter(const QString &threadFilter);
        [[nodiscard]] const QString &categoryFilter() const;
        void setCategoryFilter(const QString &categoryFilter);

    signals:
        void filterTextChanged(const QString &filterText);
        void minimumDurationMicrosecondsChanged(qint64 minimumDurationMicroseconds);
        void threadFilterChanged(const QString &threadFilter);
        void categoryFilterChanged(const QString &categoryFilter);

    private:
        QString filterText_;
        qint64 minimumDurationMicroseconds_ = 0;
        QString threadFilter_;
        QString categoryFilter_;
    };

} // namespace tracegraph::app
