---
name: brave/current-year-in-new-file-copyright
description:
  'The copyright header of a new file carries the current year, taken from the
  system date, never a year copied from another file.'
rubric: correctness
severity: should_fix
---

**Rule.** The copyright header of a file the change adds MUST carry the current
year. Take the year from the system date (`date +%Y`), NEVER from memory or
training data, and never from the file the new one was modelled on. This applies
to new Brave files; a file copied from Chromium keeps Chromium's notice and year
(see `brave/keep-chromium-copyright-on-derived-code`).

**Why.** The year records when the file was created. A year copied from another
file, or a model's training cutoff, is often years out of date.

## Examples

Bad:

```cpp
// A file added in 2026, its header copied from an older file.
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
```

Good:

```cpp
/* Copyright (c) 2026 The Brave Authors. All rights reserved.
```
