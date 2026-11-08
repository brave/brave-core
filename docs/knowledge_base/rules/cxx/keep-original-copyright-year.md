---
name: brave/keep-original-copyright-year
description:
  'An existing copyright header, or one copied with Chromium code, keeps its
  original year; a change never bumps it.'
rubric: correctness
severity: should_fix
---

**Rule.** Do not change the year of an existing copyright notice, whether in a
file the change edits, moves or renames, or in Chromium code a change copies.
The year is the one the code was first written in.

**Why.** The year records when the work was created; updating it on edit
misstates that, and adds an unrelated line to the diff.

## Examples

Bad:

```cpp
-// Copyright 2019 The Chromium Authors
+// Copyright 2026 The Chromium Authors
```

Good:

```cpp
 // Copyright 2019 The Chromium Authors
```
