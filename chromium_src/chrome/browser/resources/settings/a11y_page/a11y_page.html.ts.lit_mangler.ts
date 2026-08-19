// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle, mangleAll } from 'lit_mangler'

// <settings-live-caption> only renders behind a
// `${this.enableLiveCaption_ ? html`...` : ''}` ternary that only exists in
// the Mac/Windows build of this file -- the surrounding
// if-expr block is stripped entirely on other platforms by if-expr
// preprocessing, which runs before this mangler. mangleAll (rather than mangle)
// is used so this is a no-op instead of a build error on platforms where the
// ternary doesn't exist.

mangleAll(
  (element) => { element.textContent = '' },
  (t) => t.text.includes('settings-live-caption'),
)

mangle((root) => {
  // #captions opens the system captions dialog (Mac/Win) or routes to the
  // Captions subpage (Linux); present on every desktop platform Brave ships.
  const captions = root.getElementById('captions')
  if (!captions) {
    throw new Error(`[Settings] Accessibility page: couldn't find #captions`)
  }
  captions.remove()

  // We don't support this service.
  const imageLabelsToggle = root.querySelector(
    'settings-toggle-button[pref-key="settings.a11y.enable_accessibility_image_labels"]')
  if (!imageLabelsToggle) {
    throw new Error(
      `[Settings] Accessibility page: couldn't find image labels toggle`)
  }
  imageLabelsToggle.remove()
})
