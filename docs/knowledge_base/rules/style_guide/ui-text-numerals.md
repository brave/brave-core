---
name: brave/ui-text-numerals
description:
  User-facing text writes numbers as digits in UI and headlines (spelling out
  0–9 only in marketing body copy), uses commas from 1,000, and spells out a
  number that starts a sentence.
rubric: readability
severity: nit
---

**Rule.** In UI and headlines, write numbers as digits ("3 tabs"). In marketing
body copy, spell out zero to nine ("three") and use digits from 10. Write 1,000
and above with thousands commas everywhere. Always spell out a number that
starts a sentence.

**Why.** Digits are faster to scan in UI; consistent rules keep numbers uniform
across surfaces.

## Examples

Bad:

```xml
<message name="IDS_TABS_CLOSED" desc="Toast after closing tabs">
  3 tabs closed. Brave blocked 1500 trackers.
</message>
```

Good:

```xml
<message name="IDS_TABS_CLOSED" desc="Toast after closing tabs">
  Three tabs closed. Brave blocked 1,500 trackers.
</message>
```
