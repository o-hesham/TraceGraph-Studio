#include "trace_reader.h"

#include <QByteArray>
#include <QFile>
#include <variant>

namespace tracegraph::io
{

    namespace
    {

        using FileReadResult = std::variant<QByteArray, domain::TraceLoadErrors>;

        [[nodiscard]] FileReadResult readFileBytes(const QString &filePath)
        {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly))
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::FileSystem,
                        file.errorString(),
                        filePath}};
            }

            const QByteArray data = file.readAll();
            if (file.error() != QFileDevice::NoError)
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::FileSystem,
                        file.errorString(),
                        filePath}};
            }

            return data;
        }

    } // anonymous namespace

} // namespace tracegraph::io
