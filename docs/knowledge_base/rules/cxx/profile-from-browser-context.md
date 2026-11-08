---
name: brave/profile-from-browser-context
description:
  'A BrowserContext* is converted to a Profile* with
  Profile::FromBrowserContext(), never a static_cast.'
rubric: safety
severity: should_fix
---

**Rule.** Get a `Profile*` from a `content::BrowserContext*` with
`Profile::FromBrowserContext()`. NEVER `static_cast` (or otherwise cast) the
pointer.

**Why.** `Profile::FromBrowserContext()` includes safety checks a cast skips, so
a context that is not a `Profile` is caught instead of being used as one.

## Examples

Bad:

```cpp
Profile* profile = static_cast<Profile*>(browser_context);
```

Good:

```cpp
Profile* profile = Profile::FromBrowserContext(browser_context);
```
