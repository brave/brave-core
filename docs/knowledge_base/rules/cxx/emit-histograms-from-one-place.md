---
name: brave/emit-histograms-from-one-place
description:
  'Each UMA or P3A histogram is emitted from a single place, a helper that
  callers share, not from several call sites.'
rubric: design
severity: should_fix
---

**Rule.** Emit each histogram from exactly one place. When several call sites
record the same histogram, give it a recording helper (`Record*()`) that owns
the histogram's name, sample type and bounds, and call that helper from each
site.

**Why.** Histogram macros cache their histogram, and each copy of the name,
bucket count and maximum must agree exactly: duplicated emission drifts, and a
mismatch corrupts or splits the data.

## Examples

Bad:

```cpp
void OnButtonClicked() {
  base::UmaHistogramExactLinear("Brave.NTP.CustomizeUsage", 2, 7);
}
```

Good:

```cpp
void RecordNTPCustomizeUsage(NTPCustomizeUsage usage) {
  base::UmaHistogramExactLinear("Brave.NTP.CustomizeUsage",
                                static_cast<int>(usage),
                                static_cast<int>(NTPCustomizeUsage::kSize));
}
```
