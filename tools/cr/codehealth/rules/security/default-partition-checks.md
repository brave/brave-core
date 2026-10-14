---
name: security/default-partition-checks
rubric: security
severity: regression
---

**Rule.** Upstream checks against the default storage partition keep Brave's
container partitions working.

**Why.** Brave exempts container partitions in
`PermissionUtil::IsPermissionBlockedInPartition`, but upstream hardening often
compares against `GetDefaultStoragePartition()` directly, which bypasses it.

**How to check.** Check each added `GetDefaultStoragePartition()` line in the
candidates against containers.

## Examples

- WebHID in container tabs.
