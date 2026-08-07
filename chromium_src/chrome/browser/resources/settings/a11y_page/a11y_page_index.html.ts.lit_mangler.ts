// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Removes the Captions subpage view; the row that used to link to it is
// removed by the companion a11y_page.html.ts.lit_mangler.ts override. This
// only exists in the Linux build of this file (the surrounding
// if-expr is stripped on other platforms by if-expr
// preprocessing, which runs before this mangler), so ?. is used instead of
// throwing when it's missing.
mangle((root) => {
  root.getElementById('captions')?.remove()
})
