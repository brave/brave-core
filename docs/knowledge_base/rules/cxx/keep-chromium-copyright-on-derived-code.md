---
name: brave/keep-chromium-copyright-on-derived-code
description:
  "A file copied or derived from Chromium code keeps Chromium's copyright
  notice; it never gets Brave's header instead."
rubric: correctness
severity: blocking
---

**Rule.** When a change adds a file copied from Chromium, or code derived from
Chromium code, the file MUST carry The Chromium Authors' copyright notice. NEVER
replace it with Brave's header.

**Why.** The copyright notice records who wrote the code and is required by its
licence; putting Brave's notice on Chromium's code misattributes it.

## Examples

Bad:

```cpp
// Copied from chrome/browser/ui/views/frame/browser_view.cc
/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. ...
```

Good:

```cpp
// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
```
