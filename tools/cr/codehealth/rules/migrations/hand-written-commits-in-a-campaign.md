---
name: migrations/hand-written-commits-in-a-campaign
rubric: migrations
severity: regression
---

**Rule.** Brave's call sites follow the hand-written commits of a campaign, not
only its scripted ones.

**Why.** The non-scripted commits under a campaign's `Bug:` change APIs rather
than rewrite them: one that drops a parameter breaks Brave's call sites, which
the scripted commits never touch.

**How to check.** In the candidate list, read each
`by hand, on a header Brave includes:` commit's header diff, and grep Brave for
the declarations it changes or removes.

## Examples

- M158, 8115.1: `ios::HistoryServiceFactory::GetForProfile()` lost its
  `ServiceAccessType`.
