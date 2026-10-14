---
name: security/fixes-in-fork-originals
rubric: security
severity: regression
---

**Rule.** A Brave fork of upstream code gets the fixes upstream makes to the
original.

**Why.** A copy doesn't follow its original: a validation, a contract change or
a type change lands upstream only. When upstream changes a contract ("factory no
longer returns null; check `IsEnabled()`"), the callers it updates are the
originals, never Brave's copies.

**How to check.** The candidate list is the output of
`tools/cr/codehealth/scripts/forks.py`: each changed original, the Brave forks
of it, and the range's commits to it. It indexes "copied from", "taken from",
"adapted from" and "kept in sync with" comments, `chromium_src` files that never
include the original, and the known pairs in `ForkIndex.KNOWN`. Read each
commit's diff (`git log -L :<function>:<file>` for a single function) and check
whether the Brave copy needs the same fix. Not indexed: `_ChromiumImpl` renames
whose Brave body never calls the original; grep those by hand. A fork found by
hand belongs in `ForkIndex.KNOWN`.

## Examples

- `BraveProxyingURLLoaderFactory`, a fork of
  `WebRequestProxyingURLLoaderFactory`: FollowRedirect validation;
  `bypass_redirect_checks`.
- `BraveSearchEnginesHandler`, a fork of `SearchEnginesHandler` (a miss, found
  in the rebase): search engine IDs became strings, and Brave's copied parser
  sent `NaN`.
- M158: Brave's `HistoryDriverIOS` fork missed the `IsEnabled()` check and now
  hits a `CHECK`.
