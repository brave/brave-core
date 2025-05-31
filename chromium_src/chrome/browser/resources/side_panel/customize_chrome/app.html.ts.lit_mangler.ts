// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Update "Customize toolbar" button's icon
mangle(
  (element) => {
    const el = element.querySelector('#toolbarButton')
    if (!el) {
      throw new Error('[Customize Chrome] #toolbarButton is gone.')
    }

    // Remove existing chevron icon
    el.querySelector('cr-icon[icon="cr:chevron-right"]')?.remove()

    // Insert new icon at the start of the button
    el.insertAdjacentHTML(
      'afterbegin',
      `<leo-icon name="window-edit" slot="prefix-icon"></leo-icon>`,
    )
  },
  (template) => template.text.includes('id="toolbarButton"'),
)

mangle(
  (element) => {
    const el = element.querySelector('#toolbar-customization-inner-heading')
    if (!el) {
      throw new Error(
        '[Customize Chrome] #toolbar-customization-inner-heading is gone.',
      )
    }

    // Override text content of the heading with our custom label
    el.textContent = '$i18n{braveCustomizeMenuToolbarLabel}'
  },
  (template) =>
    template.text.includes('id="toolbar-customization-inner-heading"'),
)
