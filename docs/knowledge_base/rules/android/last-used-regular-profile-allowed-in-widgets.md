---
name: brave/last-used-regular-profile-allowed-in-widgets
description:
  ProfileManager.getLastUsedRegularProfile() in an Android widget provider,
  which has no Activity or WebContents to get a Profile from, is acceptable and
  is never flagged.
rubric: design
severity: optional
---

**Rule.** NEVER flag a call to `ProfileManager.getLastUsedRegularProfile()` as a
banned-pattern violation in an Android widget provider (such as
`QuickActionSearchAndBookmarkWidgetProvider`, or the factories and services it
drives) where no Activity or WebContents is available to get a Profile from,
even though the presubmit warns about it. Elsewhere the presubmit's ban stands:
the Profile is passed in or taken from an entity that has one.

**Why.** A widget is a top-level entry point with no Activity or WebContents, so
the last used regular profile is the only Profile it can reach. Upstream
Chromium's own widgets do the same.

## Examples

Good:

```java
public class QuickActionSearchAndBookmarkWidgetProvider extends AppWidgetProvider {
    private static void updateTiles() {
        Profile profile = ProfileManager.getLastUsedRegularProfile();
        // ...
    }
}
```

Bad (outside a widget, where an Activity provides the Profile):

```java
Profile profile = ProfileManager.getLastUsedRegularProfile();
```
