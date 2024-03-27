# Page Graph Changelog

This document shows all the changes and improvements made in each version of
[Page Graph](https://github.com/brave/brave-browser/wiki/PageGraph).

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
