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

    // Replace existing chevron icon with a new icon and put it at the start
    // of the button.
    const icon = el.querySelector('cr-icon[icon="cr:chevron-right"]')
    if (!icon) {
      throw new Error(
        '[Customize Chrome] #toolbarButton does not have a chevron icon.',
      )
    }
    icon.setAttribute('icon', 'window-edit')
    icon.setAttribute('slot', 'prefix-icon')
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

// Insert a close button into the sp-heading element.
mangle(
  (element: DocumentFragment) => {
    // Remove the "Choose which icons to show on the toolbar" text.
    const el = element.querySelector('sp-heading')
    if (!el) {
      throw new Error('[Customize Chrome] sp-heading is gone.')
    }

    el.insertAdjacentHTML(
      'afterbegin',
      /* html */ `
      <close-panel-button id="closeButton" iron-icon="close" slot="buttons"/>`,
    )
  },
  (template) => template.text.includes('sp-heading'),
)
