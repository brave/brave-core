---
name: brave/record-prefix-for-metrics-methods
description:
  'A method that records a UMA histogram or P3A metric is named Record*, not
  Send*, Report* or similar.'
rubric: readability
severity: nit
---

**Rule.** Name a function or method whose job is to record a histogram or P3A
metric `Record*()`: `RecordSavingsDaily()`, not `SendSavingsDaily()`,
`ReportSavingsDaily()` or `LogSavingsDaily()`.

**Why.** A conventional prefix tells the reader what the method does without
opening it, and `Send*` or `Report*` can be mistaken for code that transmits
data. With one prefix, the metrics a class records are easy to find.

## Examples

Bad:

```cpp
void SendSavingsDaily();
```

Good:

```cpp
void RecordSavingsDaily();
```
