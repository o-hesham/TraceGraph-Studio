#include "trace_reader.h"

#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonValue>
#include <QSet>
#include <QVector>
#include <QHash>
#include <variant>
#include <optional>
#include <utility>

namespace tracegraph::io
{

    namespace
    {
        enum class VisitState
        {
            Unvisited,
            Visiting,
            Visited
        };

        using FileReadResult = std::variant<QByteArray, domain::TraceLoadErrors>;
        using RootParseResult = std::variant<QJsonObject, domain::TraceLoadErrors>;
        using RootValidationResult = std::variant<QJsonArray, domain::TraceLoadErrors>;
        using IntegerFieldResult = std::variant<qint64, domain::TraceLoadError>;
        using StringFieldResult = std::variant<QString, domain::TraceLoadError>;
        using EventFieldsValidationResult = std::optional<domain::TraceLoadError>;
        using OptionalEventIdResult = std::variant<std::optional<domain::EventId>, domain::TraceLoadError>;
        using DependenciesResult = std::variant<QVector<domain::EventId>, domain::TraceLoadError>;
        using EventParserResult = std::variant<domain::TraceEvent, domain::TraceLoadError>;
        using EventsParseResult = std::variant<QVector<domain::TraceEvent>, domain::TraceLoadErrors>;
        using ReferenceValidationResult = std::optional<domain::TraceLoadError>;
        using ParentCycleValidationResult = std::optional<domain::TraceLoadError>;
        using DependencyCycleValidationResult = std::optional<domain::TraceLoadError>;

        // Reads an entire file and returns either its bytes or a file-system error.
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

        // Parses UTF-8 JSON bytes and requires the document root to be an object.
        [[nodiscard]] RootParseResult parseRootObject(const QByteArray &data)
        {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError)
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::MalformedJson,
                        parseError.errorString(),
                        QStringLiteral("byte %1").arg(parseError.offset)}};
            }

            if (!document.isObject())
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::Schema,
                        "Root JSON value must be an object.",
                        "$"}};
            }

            return document.object();
        }

        // Validates the .tgtrace root contract and returns its events array.
        [[nodiscard]] RootValidationResult validateRoot(const QJsonObject &root)
        {
            static const QSet<QString> allowedFields{
                "schemaVersion",
                "timeUnit",
                "events"};

            // Reject unknown keys
            for (auto it = root.constBegin(); it != root.constEnd(); ++it)
            {
                const QString &key = it.key();

                if (!allowedFields.contains(key))
                {
                    return domain::TraceLoadErrors{
                        domain::TraceLoadError{
                            domain::TraceErrorCategory::Schema,
                            QStringLiteral("Unknown root field: %1").arg(key),
                            key}};
                }
            }

            const auto schemaVersion = root.value(QStringLiteral("schemaVersion"));
            const auto timeUnit = root.value(QStringLiteral("timeUnit"));
            const auto events = root.value(QStringLiteral("events"));

            if (!schemaVersion.isDouble() || schemaVersion.toInteger(-1) != 1)
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::Schema,
                        QStringLiteral("schemaVersion must be the integer 1."),
                        "schemaVersion"}};
            }
            if (!timeUnit.isString() || timeUnit.toString() != "microseconds")
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::Schema,
                        QStringLiteral("timeUnit must be \"microseconds\"."),
                        "timeUnit"}};
            }
            if (!events.isArray())
            {
                return domain::TraceLoadErrors{
                    domain::TraceLoadError{
                        domain::TraceErrorCategory::Schema,
                        QStringLiteral("events must be an array."),
                        "events"}};
            }

            return events.toArray();
        }

        // Reads a required integer field and enforces its minimum allowed value.
        [[nodiscard]] IntegerFieldResult readRequiredInteger(const QJsonObject &object, const QString &fieldName, qint64 minimum, const QString &location)
        {
            const auto fieldValue = object.value(fieldName);
            if (!fieldValue.isDouble())
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("%1 must be an integer.").arg(fieldName),
                    location};
            }

            const qint64 result = fieldValue.toInteger(-1);
            if (result < minimum)
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("%1 must be an integer greater than or equal to %2.").arg(fieldName).arg(minimum),
                    location};
            }

            return result;
        }

        // Reads a required string field and rejects empty values.
        [[nodiscard]] StringFieldResult readRequiredString(const QJsonObject &object, const QString &fieldName, const QString &location)
        {
            const auto fieldValue = object.value(fieldName);
            if (!fieldValue.isString())
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("%1 must be a string.").arg(fieldName),
                    location};
            }

            const QString result = fieldValue.toString();
            if (result.isEmpty())
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("%1 must not be empty.").arg(fieldName),
                    location};
            }

            return result;
        }

        // Builds the JSON location for a field within an event object.
        [[nodiscard]] QString eventFieldLocation(qsizetype eventIndex, const QString &fieldName)
        {
            return QStringLiteral("events[%1].%2").arg(eventIndex).arg(fieldName);
        }

        // Rejects fields that are not part of the version 1 event contract.
        [[nodiscard]] EventFieldsValidationResult validateEventFields(const QJsonObject &object, qsizetype eventIndex)
        {
            static const QSet<QString> allowedFields{
                QStringLiteral("id"),
                QStringLiteral("name"),
                QStringLiteral("category"),
                QStringLiteral("thread"),
                QStringLiteral("start"),
                QStringLiteral("duration"),
                QStringLiteral("parentId"),
                QStringLiteral("dependsOn")};

            for (auto it = object.constBegin(); it != object.constEnd(); ++it)
            {
                const auto key = it.key();

                if (!allowedFields.contains(key))
                {
                    return domain::TraceLoadError{
                        domain::TraceErrorCategory::Field,
                        QStringLiteral("Unknown event field: %1").arg(key),
                        eventFieldLocation(eventIndex, key)};
                }
            }

            return std::nullopt;
        }

        // Reads an optional positive event ID; absence produces an empty optional.
        [[nodiscard]] OptionalEventIdResult readOptionalEventId(const QJsonObject &object,
                                                                const QString &fieldName, const QString &location)
        {
            if (!object.contains(fieldName))
            {
                return std::optional<domain::EventId>{};
            }

            const auto integerResult = readRequiredInteger(object, fieldName, 1, location);
            if (const auto *error = std::get_if<domain::TraceLoadError>(&integerResult))
            {
                return *error;
            }

            return std::optional<domain::EventId>{
                std::get<qint64>(integerResult)};
        }

        // Reads the optional dependsOn array and validates its event IDs.
        [[nodiscard]] DependenciesResult readDependencies(const QJsonObject &object, qsizetype eventIndex)
        {
            const auto fieldName = QStringLiteral("dependsOn");
            if (!object.contains(fieldName))
            {
                return QVector<domain::EventId>{};
            }

            const auto fieldValue = object.value(fieldName);
            if (!fieldValue.isArray())
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("%1 must be an array.").arg(fieldName),
                    eventFieldLocation(eventIndex, fieldName)};
            }

            const QJsonArray dependsOnArray = fieldValue.toArray();

            QVector<domain::EventId> dependencies;
            dependencies.reserve(dependsOnArray.size());
            QSet<domain::EventId> seenIds;

            for (qsizetype arrayIndex = 0; arrayIndex < dependsOnArray.size(); ++arrayIndex)
            {
                const QString location = QStringLiteral("events[%1].dependsOn[%2]").arg(eventIndex).arg(arrayIndex);
                const QJsonValue dependencyValue = dependsOnArray.at(arrayIndex);

                if (!dependencyValue.isDouble())
                {
                    return domain::TraceLoadError{
                        domain::TraceErrorCategory::Field,
                        QStringLiteral("Dependency IDs must be positive integers."),
                        location};
                }

                const auto dependencyID = dependencyValue.toInteger(-1);
                if (dependencyID < 1)
                {
                    return domain::TraceLoadError{
                        domain::TraceErrorCategory::Field,
                        QStringLiteral("Dependency IDs must be positive integers."),
                        location};
                }

                if (seenIds.contains(dependencyID))
                {
                    return domain::TraceLoadError{
                        domain::TraceErrorCategory::Field,
                        QStringLiteral("dependsOn must not contain duplicate IDs."),
                        location};
                }

                seenIds.insert(dependencyID);
                dependencies.append(dependencyID);
            }

            return dependencies;
        }

        // Parses and validates one JSON event into a TraceEvent.
        [[nodiscard]] EventParserResult parseEvent(const QJsonValue &value, qsizetype eventIndex)
        {
            if (!value.isObject())
            {
                return domain::TraceLoadError{
                    domain::TraceErrorCategory::Field,
                    QStringLiteral("events[%1] must be an object.").arg(eventIndex),
                    QStringLiteral("events[%1]").arg(eventIndex)};
            }

            const auto object = value.toObject();
            const auto fieldsValidation = validateEventFields(object, eventIndex);
            if (fieldsValidation)
            {
                return *fieldsValidation;
            }

            const auto idResult = readRequiredInteger(object, QStringLiteral("id"), 1, eventFieldLocation(eventIndex, QStringLiteral("id")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&idResult))
            {
                return *error;
            }

            const auto nameResult = readRequiredString(object, QStringLiteral("name"), eventFieldLocation(eventIndex, QStringLiteral("name")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&nameResult))
            {
                return *error;
            }

            const auto categoryResult = readRequiredString(object, QStringLiteral("category"), eventFieldLocation(eventIndex, QStringLiteral("category")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&categoryResult))
            {
                return *error;
            }

            const auto threadResult = readRequiredString(object, QStringLiteral("thread"), eventFieldLocation(eventIndex, QStringLiteral("thread")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&threadResult))
            {
                return *error;
            }

            const auto startResult = readRequiredInteger(object, QStringLiteral("start"), 0, eventFieldLocation(eventIndex, QStringLiteral("start")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&startResult))
            {
                return *error;
            }

            const auto durationResult = readRequiredInteger(object, QStringLiteral("duration"), 1, eventFieldLocation(eventIndex, QStringLiteral("duration")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&durationResult))
            {
                return *error;
            }

            const auto parentIdResult = readOptionalEventId(object, QStringLiteral("parentId"), eventFieldLocation(eventIndex, QStringLiteral("parentId")));
            if (const auto *error = std::get_if<domain::TraceLoadError>(&parentIdResult))
            {
                return *error;
            }

            const auto dependenciesResult = readDependencies(object, eventIndex);
            if (const auto *error = std::get_if<domain::TraceLoadError>(&dependenciesResult))
            {
                return *error;
            }

            domain::TraceEvent event{};
            event.id = std::get<qint64>(idResult);
            event.name = std::get<QString>(nameResult);
            event.category = std::get<QString>(categoryResult);
            event.thread = std::get<QString>(threadResult);
            event.startMicroseconds = std::get<qint64>(startResult);
            event.durationMicroseconds = std::get<qint64>(durationResult);
            event.parentId = std::get<std::optional<domain::EventId>>(parentIdResult);
            event.dependencies = std::get<QVector<domain::EventId>>(dependenciesResult);

            return event;
        }

        // Parses every event and rejects duplicate event IDs.
        [[nodiscard]] EventsParseResult parseEvents(const QJsonArray &eventValues)
        {
            QVector<domain::TraceEvent> events;
            events.reserve(eventValues.size());

            QSet<domain::EventId> seenIds;

            for (qsizetype arrayIndex = 0; arrayIndex < eventValues.size(); ++arrayIndex)
            {
                const auto eventResult = parseEvent(eventValues.at(arrayIndex), arrayIndex);
                if (const auto *error = std::get_if<domain::TraceLoadError>(&eventResult))
                {
                    return domain::TraceLoadErrors{*error};
                }

                const auto event = std::get<domain::TraceEvent>(eventResult);
                if (seenIds.contains(event.id))
                {
                    return domain::TraceLoadErrors{domain::TraceLoadError{
                        domain::TraceErrorCategory::Field,
                        QStringLiteral("Duplicate event ID: %1.").arg(event.id),
                        eventFieldLocation(arrayIndex, QStringLiteral("id"))}};
                }

                seenIds.insert(event.id);
                events.append(event);
            }

            return events;
        }

        // Validates that parent and dependency references exist and are not self-references.
        [[nodiscard]] ReferenceValidationResult validateReferences(const QVector<domain::TraceEvent> &events)
        {
            QSet<domain::EventId> eventIds;
            eventIds.reserve(events.size());

            for (const auto &event : events)
            {
                eventIds.insert(event.id);
            }

            for (qsizetype eventIndex = 0; eventIndex < events.size(); ++eventIndex)
            {
                const auto &event = events.at(eventIndex);
                if (event.parentId.has_value())
                {
                    const auto parentId = *event.parentId;

                    if (parentId == event.id)
                    {
                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Reference,
                            QStringLiteral("An event cannot be its own parent."),
                            eventFieldLocation(eventIndex, QStringLiteral("parentId"))};
                    }

                    if (!eventIds.contains(parentId))
                    {
                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Reference,
                            QStringLiteral("parentId references unknown event ID: %1.").arg(parentId),
                            eventFieldLocation(eventIndex, QStringLiteral("parentId"))};
                    }
                }

                for (qsizetype dependencyIndex = 0; dependencyIndex < event.dependencies.size(); dependencyIndex++)
                {
                    const auto dependencyId = event.dependencies.at(dependencyIndex);
                    const QString location = QStringLiteral("events[%1].dependsOn[%2]").arg(eventIndex).arg(dependencyIndex);

                    if (dependencyId == event.id)
                    {
                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Reference,
                            QStringLiteral("An event cannot depend on itself."),
                            location};
                    }

                    if (!eventIds.contains(dependencyId))
                    {
                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Reference,
                            QStringLiteral("dependsOn references unknown event ID: %1.").arg(dependencyId),
                            location};
                    }
                }
            }

            return std::nullopt;
        }

        // Detects cycles in parent relationships.
        [[nodiscard]] ParentCycleValidationResult validateParentCycles(const QVector<domain::TraceEvent> &events)
        {
            QHash<domain::EventId, qsizetype> eventIndexById;
            eventIndexById.reserve(events.size());

            for (qsizetype eventIndex = 0; eventIndex < events.size(); ++eventIndex)
            {
                eventIndexById.insert(events.at(eventIndex).id, eventIndex);
            }

            QVector<VisitState> visitStates(events.size(), VisitState::Unvisited);

            for (qsizetype startIndex = 0; startIndex < events.size(); ++startIndex)
            {
                if (visitStates.at(startIndex) != VisitState::Unvisited)
                {
                    continue;
                }

                QVector<qsizetype> path;
                qsizetype currentIndex = startIndex;

                while (true)
                {
                    if (visitStates.at(currentIndex) == VisitState::Visited)
                    {
                        break;
                    }

                    if (visitStates.at(currentIndex) == VisitState::Visiting)
                    {
                        const qsizetype sourceIndex = path.constLast();

                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Cycle,
                            QStringLiteral("Parent relationships contain a cycle."),
                            eventFieldLocation(sourceIndex, QStringLiteral("parentId"))};
                    }

                    visitStates[currentIndex] = VisitState::Visiting;
                    path.append(currentIndex);

                    const auto &event = events.at(currentIndex);

                    if (!event.parentId.has_value())
                    {
                        break;
                    }

                    currentIndex = eventIndexById.value(*event.parentId);
                }

                for (const qsizetype index : path)
                {
                    visitStates[index] = VisitState::Visited;
                }
            }

            return std::nullopt;
        }

        // Detects cycles in dependency relationships without recursion.
        [[nodiscard]] DependencyCycleValidationResult validateDependencyCycles(const QVector<domain::TraceEvent> &events)
        {
            struct Frame
            {
                qsizetype eventIndex;
                qsizetype nextDependencyIndex;
            };

            QHash<domain::EventId, qsizetype> eventIndexById;
            eventIndexById.reserve(events.size());

            for (qsizetype eventIndex = 0; eventIndex < events.size(); ++eventIndex)
            {
                eventIndexById.insert(events.at(eventIndex).id, eventIndex);
            }

            QVector<VisitState> visitStates(events.size(), VisitState::Unvisited);
            QVector<Frame> stack;
            stack.reserve(events.size());

            for (qsizetype startIndex = 0; startIndex < events.size(); ++startIndex)
            {
                if (visitStates.at(startIndex) != VisitState::Unvisited)
                {
                    continue;
                }

                stack.clear();
                visitStates[startIndex] = VisitState::Visiting;
                stack.push_back(Frame{startIndex, 0});

                while (!stack.empty())
                {
                    Frame &frame = stack.last();
                    const auto &event = events.at(frame.eventIndex);

                    if (frame.nextDependencyIndex >= event.dependencies.size())
                    {
                        visitStates[frame.eventIndex] = VisitState::Visited;

                        stack.removeLast();
                        continue;
                    }

                    const qsizetype dependencyIndex = frame.nextDependencyIndex;

                    ++frame.nextDependencyIndex;

                    const auto dependencyId = event.dependencies.at(dependencyIndex);

                    const qsizetype dependencyEventIndex = eventIndexById.value(dependencyId);

                    const VisitState dependencyState = visitStates.at(dependencyEventIndex);

                    if (dependencyState == VisitState::Visiting)
                    {
                        return domain::TraceLoadError{
                            domain::TraceErrorCategory::Cycle,
                            QStringLiteral("Dependency relationships contain a cycle."),
                            QStringLiteral("events[%1].dependsOn[%2]").arg(frame.eventIndex).arg(dependencyIndex)};
                    }

                    if (dependencyState == VisitState::Unvisited)
                    {
                        visitStates[dependencyEventIndex] = VisitState::Visiting;

                        stack.push_back(Frame{dependencyEventIndex, 0});
                    }
                }
            }

            return std::nullopt;
        }
    } // namespace

    // Reads and validates a trace file as one all-or-nothing operation.
    domain::TraceLoadResult TraceReader::readFile(const QString &filePath) const
    {
        auto fileResult = readFileBytes(filePath);
        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&fileResult))
        {
            return *errors;
        }
        QByteArray data = std::get<QByteArray>(std::move(fileResult));

        auto rootParseResult = parseRootObject(data);
        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&rootParseResult))
        {
            return *errors;
        }
        QJsonObject root = std::get<QJsonObject>(std::move(rootParseResult));

        auto rootValidationResult = validateRoot(root);
        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&rootValidationResult))
        {
            return *errors;
        }
        QJsonArray eventValues = std::get<QJsonArray>(std::move(rootValidationResult));

        auto eventsParseResult = parseEvents(eventValues);
        if (const auto *errors = std::get_if<domain::TraceLoadErrors>(&eventsParseResult))
        {
            return *errors;
        }
        QVector<domain::TraceEvent> events = std::get<QVector<domain::TraceEvent>>(std::move(eventsParseResult));

        if (const auto error = validateReferences(events))
        {
            return domain::TraceLoadErrors{*error};
        }

        if (const auto error = validateParentCycles(events))
        {
            return domain::TraceLoadErrors{*error};
        }

        if (const auto error = validateDependencyCycles(events))
        {
            return domain::TraceLoadErrors{*error};
        }

        return domain::TraceSession{std::move(events)};
    }

} // namespace tracegraph::io
