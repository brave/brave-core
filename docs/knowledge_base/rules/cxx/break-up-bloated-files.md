---
name: brave/break-up-bloated-files
description:
  'New functionality is not added to an already-large file or class when it can
  live in a smaller, focused helper file and target.'
rubric: design
severity: should_fix
---

**Rule.** When a change adds a distinct piece of functionality (metrics
recording, a parser, a helper subsystem) to a file or class that is already
large, put it in its own focused file, and target where appropriate, and call it
from there, instead of adding more methods to the large file.

**Why.** Large files are hard to read and to review, collect rebase conflicts,
and blur dependencies: everything in the target depends on everything the file
includes.

## Examples

Bad:

```cpp
// Adding more P3A code to the 5000-line rewards_service_impl.cc.
void RewardsServiceImpl::RecordP3AMetric1() { ... }
void RewardsServiceImpl::RecordP3AMetric2() { ... }
```

Good:

```cpp
// brave/browser/p3a/p3a_core_metrics.h
void RecordRewardsP3A(RewardsService* service);
```
