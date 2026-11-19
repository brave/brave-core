---
name: brave/ui-agnostic-mojom-interface-names
description:
  'Mojom interfaces that different UI frameworks (WebUI, native iOS) could use
  get neutral names such as FooUIHandler, not FooPageHandler.'
rubric: readability
severity: nit
---

**Rule.** Name a new mojom interface that another UI framework, such as Brave's
native iOS UI, could also consume after its role, such as `HistoryUIHandler`. Do
not use the WebUI-specific `PageHandler` naming for it.

**Why.** A `PageHandler` name implies a web-only interface, which misleads the
native clients sharing it and is inconsistent with the other shared interfaces.

## Examples

Bad:

```mojom
interface HistoryPageHandler { ... };
```

Good:

```mojom
interface HistoryUIHandler { ... };
```
