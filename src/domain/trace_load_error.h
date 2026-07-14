#pragma once

#include <QString>

namespace tracegraph::domain
{

    enum class TraceErrorCategory
    {
        FileSystem,
        MalformedJson,
        Schema,
        Field,
        Reference,
        Cycle
    };

    struct TraceLoadError
    {
        TraceErrorCategory category;
        QString message;
        QString location;
    };

} // namespace tracegraph::domain
