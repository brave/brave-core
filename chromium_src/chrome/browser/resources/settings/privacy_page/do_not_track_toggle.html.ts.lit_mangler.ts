// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  // Separates the toggle from the row above it on Brave's privacy page.
  const toggle = root.getElementById('toggle')
  if (!toggle) {
    throw new Error(`[Settings] Do Not Track toggle: couldn't find #toggle`)
  }
  toggle.classList.add('hr')
})
