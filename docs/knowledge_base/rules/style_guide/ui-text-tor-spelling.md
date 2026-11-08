---
name: brave/ui-text-tor-spelling
description: Tor is written "Tor" in user-facing text, never "TOR".
rubric: readability
severity: should_fix
---

**Rule.** Write "Tor", NEVER "TOR" (or "tor").

**Why.** Tor is the project's name, not an acronym.

## Examples

Bad:

```xml
<message name="IDS_NEW_TOR_WINDOW" desc="Menu item that opens a Tor window">
  New private window with TOR
</message>
```

Good:

```xml
<message name="IDS_NEW_TOR_WINDOW" desc="Menu item that opens a Tor window">
  New private window with Tor
</message>
```
