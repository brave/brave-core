---
name: brave/observer-notifications-for-ui-updates
description:
  'A service does not query its state to update the UI; it notifies observers,
  and the UI layer queries what it needs.'
rubric: design
severity: should_fix
---

**Rule.** When a service's state changes, it notifies its observers. It does not
run queries on the UI's behalf (such as fetching a total for a page to display)
as part of the change. The UI layer, as an observer, makes the queries it needs
in its observer method.

**Why.** A service that drives the UI knows about every surface that displays
its data, and has to change whenever one does. Observers keep the dependency
pointing from the UI to the service.

## Examples

Bad:

```cpp
void RewardsService::SavePendingContribution(...) {
  SaveToDB(...);
  GetPendingContributionsTotal();  // Updates the UI from the service.
}
```

Good:

```cpp
void RewardsService::SavePendingContribution(...) {
  SaveToDB(...);
  observers_.Notify(&Observer::OnPendingContributionSaved);
}
// The UI layer calls GetPendingContributionsTotal() in its observer method.
```
