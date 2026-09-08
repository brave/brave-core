// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import {
  SettingsClearBrowsingDataTimePickerElement
} from './clear_browsing_data_time_picker-chromium.js'

injectStyle(SettingsClearBrowsingDataTimePickerElement, css`
  :host {
    /* Dropdown menu styling */
    --cr-menu-border-radius: 8px;

    --color-chip-background-selected: var(--leo-color-button-background);
    --color-chip-foreground-selected: var(--leo-color-primary-20);
    --color-chip-foreground: var(--leo-color-text-interactive);
    --color-chip-icon: var(--leo-color-icon-interactive);
    --color-chip-icon-selected: var(--leo-color-primary-20);
    --color-chip-border: var(--leo-color-divider-interactive);
  }

  cr-chip {
    --cr-chip-border-radius: var(--leo-radius-full);
    --cr-chip-height: 38px;
    --cr-chip-padding-inline: var(--leo-spacing-l);
    font: var(--leo-font-large-regular);
  }
`)

// Brave uses Leo's caret for the "More" chip. This belongs in the companion
// lit_mangler override, but #moreButton sits in the root template, which can't
// be mangled without corrupting the nested html`` template inside
// cr-lazy-render-lit's `.template` attribute -- see the note there. The icon is
// a static attribute in the template, so Lit never rewrites it after the first
// render and setting it once here is enough.
//
// `firstUpdated` is `protected` on ReactiveElement, so reach it through an
// untyped view of the prototype to patch it from outside the class hierarchy.
const proto = SettingsClearBrowsingDataTimePickerElement.prototype as unknown as
  {
    firstUpdated?: (changedProperties: PropertyValues) => void
  }
const originalFirstUpdated = proto.firstUpdated
proto.firstUpdated = function (
  this: SettingsClearBrowsingDataTimePickerElement,
  changedProperties: PropertyValues) {
  originalFirstUpdated?.call(this, changedProperties)

  const moreButtonIcon = this.shadowRoot.querySelector('#moreButton cr-icon')
  if (!moreButtonIcon) {
    console.error(`[Settings] Time picker: couldn't find #moreButton's icon`)
    return
  }
  moreButtonIcon.setAttribute('icon', 'carat-down')
}

export * from './clear_browsing_data_time_picker-chromium.js'
