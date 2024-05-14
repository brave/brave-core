# Page Graph Changelog

This document shows all the changes and improvements made in each version of
[Page Graph](https://github.com/brave/brave-browser/wiki/PageGraph).

## Version 0.6.2

Fix error where for JS calls, we were recording the script's execution context
for the `frame id`, and not the receiver.

Remove `NormalizeUrl` function, which is no longer used or needed since we
more accurately track resources by Blink's `resource_id`, and no longer
best-effort match by URL.

## Version 0.6.1

Remove `frame id` from `NodeDOMRoot` instances. The `frame id` property
on other nodes and edges should reference the `blink id` property
on `NodeDOMRoot`.

## Version 0.6.0

#### Tracking Frame ID For Many Edges / Actions

Add `frame id` properties for most edges, to resolve ambiguities in the
graph when scripts modify or call API's in other frames (e.g., local frames)

## Version 0.5.1

#### Fix intermittent issues

Remove `ScriptState::Scope` calls from args serialization. It fails in some edge
cases, but it doesn't seem to be required at all right now.

Disable v8 cache for external scripts. The cache should be disabled for most
scripts, but here it was still active.

## Version 0.5.0

#### Tracking resources redirects

The graph now accurately depicts request redirects. When a redirect is
identified, resource nodes can now establish connections to other resource nodes
using the `request redirect` edge.

## Version 0.4.0

#### Better handling of WebAPI values

WebAPI values are now serialized more effectively. It includes all the
properties of objects, even the ones that are not easily visible. This makes it
similar to how DevTools handles object properties.

The `EdgeJSCall::args` is now a JSON array that contains all the observed
arguments. The `EdgeJSResult::result` is also a JSON value.

#### Improved tracking of Console API

The `Console` built-in functionality in V8 is now being tracked. This means that
all `console.*` APIs will be visible in the graph as JSBuiltin `ConsoleLog`,
`ConsoleWarn`, and similar.

The `console.log` WebAPI has been renamed to `ConsoleMessageAdded` to better
represent its tracking functionality. However, it may not appear in most graphs
as it is usually not bound to an acting script, and such calls are currently
ignored in the graph.

#### Expanded tracking of APIs

The `Geolocation` and `MediaDevices` WebAPIs are now being tracked.
