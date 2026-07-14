#pragma once

#include "trace_load_error.h"
#include "trace_session.h"

#include <variant>
#include <QVector>

namespace tracegraph::domain
{

    using TraceLoadErrors = QVector<TraceLoadError>;
    using TraceLoadResult = std::variant<TraceSession, TraceLoadErrors>;

} // namespace tracegraph::domain
