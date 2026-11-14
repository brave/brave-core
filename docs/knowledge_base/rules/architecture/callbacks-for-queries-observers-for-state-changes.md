---
name: brave/callbacks-for-queries-observers-for-state-changes
description:
  'Query results (Get/Fetch) are returned through callbacks; observer methods
  fire only for state changes (Set/Create/Delete), never for query responses.'
rubric: design
severity: blocking
---

**Rule.** A query method (`Get*`, `Fetch*`) returns its result through a
callback parameter. It MUST NOT deliver the result by notifying observers.
Observer notifications are only for state changes (`Set*`, `Create*`,
`Delete*`).

**Why.** Observers that receive query results get responses to requests they
never made, and the caller cannot tell which notification answers its query.

## Examples

Bad:

```cpp
void RewardsService::GetRecurringDonations() {
  engine_->GetRecurringDonations([this](auto list) {
    for (auto& observer : observers_) {
      observer.OnRecurringDonationsList(list);
    }
  });
}
```

Good:

```cpp
void RewardsService::GetRecurringDonations(GetDonationsCallback callback) {
  engine_->GetRecurringDonations(std::move(callback));
}

void RewardsService::SetRecurringDonation(double amount) {
  SaveToDB(amount);
  for (auto& observer : observers_) {
    observer.OnRecurringDonationUpdated();
  }
}
```
