---
name: defaults/stub-on-a-path-upstream-retires
rubric: defaults
severity: deprecation
---

**Rule.** When Brave disables upstream data or behaviour by stubbing one
receiver, check that upstream sends it nowhere else, and that the receiver isn't
the path upstream plans to drop.

**Why.** Brave's stub sits at one choke point. Upstream migrates by sending the
same data to a second consumer first, then removing the old path once the new
one is ready, and announces it in a TODO. Until then the stub looks fine; after,
it covers nothing, and the new consumer acts on what Brave meant to block.

**How to check.** The candidates add a "remove once … no longer needed" TODO.
Read the code around it: find where the data now also goes, and grep Brave for a
stub on the old receiver (`_Unused` `#define`s, plasters returning early,
`chromium_src` replacements). If one exists, list the new consumer and the
planned removal.

## Examples

- M159, 8128.0: 91957a1ebd42 also sends component key pins to the cert verifier,
  while Brave stubs only `NetworkService::UpdateKeyPinsList`; its TODO says to
  stop sending them to the network service.
