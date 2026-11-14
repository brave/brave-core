---
name: brave/cache-system-service-lookups-off-hot-paths
description:
  A value computed by fetching system services such as PackageManager or
  AppOpsManager is computed once and stored in a field instead of recomputed on
  a hot path, unless it can change without notification.
rubric: perf
severity: should_fix
---

**Rule.** Do not call a method that fetches system services (`PackageManager`,
`AppOpsManager`, `getSystemService()`) repeatedly on a hot path such as
`onResume()`. Compute its value once and store it in a member variable. NEVER
cache a value that can change without notification in multi-window or
configuration-change scenarios: PiP availability, for example, changes when a
second app starts.

**Why.** Each system service lookup is an expensive call into the system; on a
path that runs often it adds up. A cached value that the system can change
behind the app's back, though, goes stale, so those values are queried each
time.

## Examples

Bad:

```java
@Override
public void onResume() {
    if (hasPipPermission()) { /* fetches PackageManager + AppOpsManager */ }
}
```

Good:

```java
private boolean mHasPipPermission;

@Override
public void onCreate() {
    mHasPipPermission = hasPipPermission();
}
```
