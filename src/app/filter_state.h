#pragma once

#include <QObject>
#include <QString>

namespace tracegraph::app
{

    class FilterState final : public QObject
    {
        Q_OBJECT

    public:
        explicit FilterState(QObject *parent = nullptr);

        [[nodiscard]] const QString &filterText() const;
        void setFilterText(const QString &filterText);

    signals:
        void filterTextChanged(const QString &filterText);

    private:
        QString filterText_;
    };

} // namespace tracegraph::app
