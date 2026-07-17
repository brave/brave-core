// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Wire the Enter-to-confirm handler (defined in profile_customization_app.ts)
// onto the profile name input. JSDOM's setAttribute rejects Lit's `@event`
// binding syntax, so the attribute is injected via outerHTML instead.
mangle(
  (fragment) => {
    const input = fragment.querySelector('#nameInput')
    if (!input) {
      throw new Error(
        '[Brave Profile Customization] Could not find #nameInput. '
          + 'Has upstream changed?',
      )
    }
    input.outerHTML = input.outerHTML.replace(
      /^<cr-input/,
      '<cr-input @keydown="${this.onNameInputKeydown_}"',
    )
  },
  (t) => t.text.includes('id="nameInput"'),
)
