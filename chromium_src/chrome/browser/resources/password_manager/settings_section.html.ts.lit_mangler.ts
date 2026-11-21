// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  const passwordToggle = root.querySelector('#passwordToggle')
  if (!passwordToggle) {
    throw new Error(
      `[Brave Password Manager Overrides] Could not find '#passwordToggle'`,
    )
  }
  // Keep the "controlled by extension" row that follows '#passwordToggle'
  // directly under it. Password manager extensions such as Bitwarden take
  // control of that pref, which is exactly when the toggle below is useful,
  // so inserting between the two would attribute the row to the wrong toggle.
  const indicator = passwordToggle.nextSibling
  if (!indicator || !indicator.textContent?.includes('.extensionId')) {
    throw new Error(
      `[Brave Password Manager Overrides] Could not find the extension `
        + `indicator after '#passwordToggle'`,
    )
  }
  // Toggle that gates Brave's built-in password autofill/save via the
  // brave.password_manager.fill_enabled pref. Turning it off suppresses both
  // the fill dropdown and the "offer to save password" prompt, so users of a
  // third-party password manager don't get two competing suggestions.
  const braveFillToggle = root.ownerDocument.createElement('template')
  braveFillToggle.innerHTML = `
  <pref-toggle-button id="braveFillToggle" class="hr"
      label="$i18n{bravePasswordManagerFillLabel}"
      sub-label="$i18n{bravePasswordManagerFillSubLabel}"
      pref-key="brave.password_manager.fill_enabled">
  </pref-toggle-button>`
  indicator.after(braveFillToggle.content)
})
