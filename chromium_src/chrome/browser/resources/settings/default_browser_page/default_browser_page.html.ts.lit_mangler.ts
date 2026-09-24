// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle, mangleAll } from 'lit_mangler'

// We wanted the nested children (without the header)
mangle((root) => {
  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(
      `[Settings] Default-browser page: couldn't find settings-section`)
  }
  section.replaceWith(...Array.from(section.childNodes))
}, (t) => t.text.includes('settings-section'))

// Both of upstream's conditionally-rendered branches mark their row `first`
// because upstream assumes it's the top of its own section, but we fold it
// into the middle of the "Get Started" section, so it needs its separator
// back. Stripping the class here, in both branches' templates, means this
// stays correct no matter which branch ends up rendered.
mangleAll((root) => {
  const row = root.querySelector('.cr-row.first')
  if (!row) {
    throw new Error(
      `[Settings] Default-browser page: couldn't find .cr-row.first`)
  }
  row.classList.remove('first')
}, (t) => t.text.includes('cr-row') && t.text.includes('first'))
