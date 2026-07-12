# TraceGraph Studio Design

## Purpose

TraceGraph Studio is a guided Qt 6 learning project and portfolio application. It opens recorded trace files and helps a user answer:

- What operation ran?
- When did it run and for how long?
- Which thread ran it?
- Which operation contained it?
- Which earlier operations did it depend on?

Progress is measured by the learner understanding each Qt concept and implementation decision, not by a fixed delivery deadline.

## Version 1 Scope

Version 1 is an offline trace analyzer. It reads a native JSON-based `.tgtrace` format and presents synchronized event-table, timeline, dependency-graph, inspector, and filtering views.

Version 1 does not include live event streaming, process attachment, breakpoints, memory inspection, SQLite storage, editable node graphs, instant events, counters, or Chrome Trace Event import. Future importers may translate external formats into the same in-memory model without changing the views.

## Development Environment

The initial local environment is:

- Windows 64-bit
- Qt 6.11.1 with Qt Widgets, Qt Concurrent, and Qt Test
- MinGW-w64 GCC 13.1
- CMake 3.30.5
- Ninja 1.12.1
- C++20

Identifiers:

- Product: `TraceGraph Studio`
- CMake project: `TraceGraphStudio`
- Executable: `tracegraph-studio`
- Native trace extension: `.tgtrace`

## Native Trace Contract

The root JSON value is an object:

```json
{
  "schemaVersion": 1,
  "timeUnit": "microseconds",
  "events": []
}
```

`schemaVersion` must be the integer `1`. `timeUnit` must be the string `"microseconds"`. `events` must be an array.
An empty `events` array is valid. Unknown root or event fields are rejected in version 1 so misspelled field names cannot be silently ignored.

Each event represents one timed operation:

```json
{
  "id": 102,
  "name": "Parse JSON",
  "category": "parsing",
  "thread": "worker-1",
  "start": 10000,
  "duration": 50000,
  "parentId": 101,
  "dependsOn": [100]
}
```

Required fields:

- `id`: positive integer, unique within the file
- `name`: non-empty string
- `category`: non-empty string
- `thread`: non-empty string
- `start`: non-negative integer in microseconds
- `duration`: positive integer in microseconds

All JSON integer values must fit in a signed 64-bit integer.

Optional fields:

- `parentId`: positive integer identifying one existing event
- `dependsOn`: array of unique positive integers identifying existing events

An absent `parentId` means that the event has no parent. An absent `dependsOn` field is equivalent to an empty array.

Validation rules:

- References must resolve after the complete file has been read.
- An event cannot reference itself.
- Parent relationships cannot contain a cycle.
- Dependency relationships cannot contain a cycle.
- A failed validation rejects the complete new file; no partial session is installed.
- Version 1 does not reject a file because parent/child intervals or dependency timing overlap. Such conditions may become analysis warnings later.

## Domain Model and Ownership

`TraceEvent` is a value type containing one validated event. It does not paint UI or emit signals.

`TraceSession` owns every event from one successfully loaded file. It also owns indexes for fast ID lookup, thread grouping, parent-to-children lookup, and dependency traversal. Construction finishes before the session is given to the GUI. Consumers receive read-only access, so table, timeline, graph, and inspector cannot create conflicting copies of the data.

`TraceReader` synchronously reads, parses, and validates native trace JSON during the early milestones. It returns a `TraceLoadResult` containing either a complete session or structured errors, never both.

Each load error contains a stable error category, a human-readable message, and the closest available JSON location, for example `events[3].parentId`.

## Application Architecture

The approved hybrid Qt Widgets architecture assigns one responsibility to each part:

- `MainWindow`: menus, actions, docks, central layout, and saved window state
- `LoadController`: coordinates file selection, loading state, success, and failure
- `TraceReader`: parsing and validation without GUI dependencies
- `TraceSession`: authoritative read-only domain data
- `EventTableModel`: adapts a session for `QTableView`
- `EventFilterProxyModel`: sorts and filters table rows
- `TimelineView`: custom `QAbstractScrollArea` rendering with `QPainter`
- `GraphBuilder`: extracts a bounded dependency neighborhood around the selected event
- `GraphLayout`: calculates graph-node positions independently from painting
- `DependencyGraphView`: displays nodes and edges with `QGraphicsView`
- `InspectorWidget`: displays details for the selected event
- `SelectionController`: owns the single selected event ID and synchronizes views
- `FilterState`: owns shared filter criteria used consistently by table and timeline

`TraceSession`, not `QAbstractItemModel`, is the authoritative domain source. Qt models and custom views are projections of that session.

## Data Flow

Loading follows an all-or-nothing flow:

```text
Open file
  -> TraceReader parses and validates
  -> TraceLoadResult contains errors or a complete TraceSession
  -> GUI installs a successful session
  -> models and views refresh from the same session
  -> SelectionController synchronizes user selection
  -> FilterState keeps visibility rules consistent
```

The first parser implementation is synchronous so parsing and validation can be tested without threading complexity. After it is correct, `LoadController` runs it with Qt Concurrent and observes completion with `QFutureWatcher`. Worker code may construct ordinary domain data but may not access widgets or GUI-owned item models. Session installation remains on the GUI thread.

## Error Handling

- File-system, malformed-JSON, schema, field, reference, and cycle failures are distinguished.
- The UI reports actionable messages rather than silently ignoring invalid values.
- If a new load fails, the previously opened valid session remains active.
- A second load cannot race with installation of the first; the loading policy will initially disable opening another file until the current load finishes.
- Closing during asynchronous loading must be safe and must not update destroyed UI objects.

## Testing Strategy

Qt Test will be enabled when the working environment is created.

Automated tests will cover:

- valid minimal and multi-event trace files
- every required field and numeric boundary
- duplicate IDs, missing references, self-references, and cycles
- `TraceSession` indexes and relationship traversal
- `EventTableModel` row, column, role, and stable-ID behavior
- filtering and mapping between source and proxy models
- selection synchronization that does not create signal loops
- coordinate conversion and hit-testing in the timeline
- graph-neighborhood extraction and layout invariants
- success, failure, and object-lifetime behavior for background loading

Small trace fixtures are explicit test inputs. Larger generated fixtures are test data used only for performance and responsiveness checks.

## Implementation Milestones

1. Create a minimal CMake project using Qt Widgets and Qt Test.
2. Implement and test `TraceEvent`, `TraceSession`, `TraceReader`, and the native trace contract.
3. Implement `EventTableModel` and table sorting/filtering.
4. Implement the custom zoomable and pannable timeline with visible-event rendering and hit-testing.
5. Implement shared selection and filtering across table, timeline, and inspector.
6. Implement bounded dependency extraction, layout, and `QGraphicsView` rendering.
7. Move the already-tested synchronous loader behind Qt Concurrent without giving worker code GUI access.
8. Add `QSettings` persistence, keyboard navigation, themes, recent files, and portfolio polish.

Each milestone follows the same learning loop: explain the concept, connect it to the application, implement a small piece, run tests, inspect mistakes, and review what was learned before continuing.

The project is intentionally implemented through milestone-sized plans rather than one enormous plan. The first implementation plan covers only the working environment and minimal executable/test harness. Domain parsing begins after that environment is verified.

## Success Criteria

Version 1 is successful when it can reliably open a valid native trace, reject an invalid one with precise errors, display synchronized event information across the table, timeline, graph, and inspector, filter without inconsistent views, perform file loading outside the GUI thread, render only visible timeline events, preserve window state, and pass its automated tests.
