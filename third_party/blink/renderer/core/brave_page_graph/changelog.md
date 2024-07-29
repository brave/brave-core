# Page Graph Changelog

This document shows all the changes and improvements made in each version of
[Page Graph](https://github.com/brave/brave-browser/wiki/PageGraph).

## Version 0.7.3

Support for SVG documents (both capturing the structure, and for `<use>`
elements, capturing and tying the request to the element).

## Version 0.7.2

Reworked how scripts are tracked in the graph. The actor type hierarchy
now looks like this:

```
NodeActor (abstract class)
  - NodeScript (abstract class)
    * NodeScriptLocal: A script thats executing in this v8 Isolate (i.e., 99%
                       of scripts).
    * NodeScriptRemote: A script thats executing in a different v8 Isolate.
                        PageGraph will only ever see these if a script
                        in another isolate injecting a script into this
                        document (e.g., Extension content scripts, puppeteer
                        scripts).
  - NodeParser: The parser node, one per document, no changes
  - NodeUnknown: New type used for explicitly recording when we can't find
                 a script that must be running. These are error cases,
                 but we use this type to explicitly identify the error
                 cases (instead of mixing them in among the correct
                 attributions).
```

## Version 0.7.1

Add `EdgeAttributeSet` instances to capture attributes that were created
by the parser as well.

Minor type tightening in method signatures.

## Version 0.7.0

Add new `EdgeDocument` type, for recording the final state of the elements
in the DOM. Note that this replaces some of the uses for `EdgeStructure`
edges, which were previously used to wire together some structure in the
graph (e.g., tying a parser to a dom root.)

Change how `EdgeCrossDOM` instances are structured. Previously they were
tied from an iframe-like-element (i.e., `NodeFrameOwner`) to the parser
for the execution context, which is confusing and was implemented
incorrectly anyway. Now we use `EdgeCrossDOM` edges tie each `NodeDOMRoot`
node directly to its "containing" node (i.e.,
`NodeFrameOwner` -> `EdgeCrossDOM` -> `NodeDOMRoot`).

Remove not actually used `NodeRemoteFrame` code and references.

Add `is attached` property to `NodeDOMRoot` instances, to explicitly
record if a frame is attached to the DOM tree, or not.

Fix fetching the `frame id` when creating `NodeDOMRoot` elements.
Before we tried to get the `frame id` off the execution context of
the newly created document itself, which was incorrect.  Instead we grab
the `frame id` of the execution context the `NodeDOMRoot` was created in
by getting its parent document.

## Version 0.6.3

Add `frame id` attribute to `EdgeExecute` and `EdgeExecuteAttr` edges too.

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
