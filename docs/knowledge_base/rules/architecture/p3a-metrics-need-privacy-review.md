---
name: brave/p3a-metrics-need-privacy-review
description:
  'A change adding P3A metrics needs a privacy review: a description of the new
  metrics posted in the p3a Slack channel before merging.'
rubric: security
severity: blocking
---

**Rule.** When a change adds a P3A metric (a new `Brave.*` histogram, or a new
entry in the P3A metric lists), it MUST NOT merge before the analytics team's
privacy review: a brief description of the new metrics posted in the p3a Slack
channel. Flag the new metric and ask for the review.

**Why.** P3A metrics leave the user's device; their strict privacy requirements
are checked by the analytics team, and a metric that slips past them can report
more than users agreed to share.

## Examples

Bad:

```cpp
// New metric, no privacy review requested.
UMA_HISTOGRAM_EXACT_LINEAR("Brave.Foo.UsageCount", count, 5);
```

Good:

```cpp
// Reviewed in #p3a: one bucketed weekly usage count, no identifiers.
UMA_HISTOGRAM_EXACT_LINEAR("Brave.Foo.UsageCount", count, 5);
```
