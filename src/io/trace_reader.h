#pragma once

#include "domain/trace_load_result.h"

#include <QString>

namespace tracegraph::io
{

    class TraceReader
    {
    public:
        [[nodiscard]] domain::TraceLoadResult readFile(const QString &filePath) const;
    };

} // namespace tracegraph::io
