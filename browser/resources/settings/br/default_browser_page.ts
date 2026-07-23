/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import {
  SettingsDefaultBrowserPageElement
} from '../default_browser_page/default_browser_page.js'

// The page's row is marked `first` because upstream assumes it's the top of
// its own section, but we fold it into the middle of the "Get Started"
// section, so it needs its separator back. This is templated directly (not a
// bound attribute), so it's reapplied any time Lit swaps in the other
// code branch (e.g., once the async default-browser state arrives).
injectStyle(SettingsDefaultBrowserPageElement, css`
  .cr-row.first {
    border-top: var(--cr-separator-line);
   }
`)

const modifyDefaultBrowserPage = (root: ShadowRoot) => {
  // Replace settings-section with its children, since we want its
  // controls in our page but not the header that it also now includes.
  const settingsSection = root.querySelector('settings-section')
  if (!settingsSection) {
    throw new Error(
      '[Settings] Missing settings-section on default_browser_page')
  }
  settingsSection.replaceWith(...settingsSection.childNodes)
}

// `firstUpdated` is `protected` on ReactiveElement, so reach it through an
// untyped view of the prototype to patch it from outside the class hierarchy.
const proto = SettingsDefaultBrowserPageElement.prototype as unknown as {
  firstUpdated?: (changedProperties: PropertyValues) => void
}
const originalFirstUpdated = proto.firstUpdated
proto.firstUpdated = function(
    this: SettingsDefaultBrowserPageElement,
    changedProperties: PropertyValues) {
  originalFirstUpdated?.call(this, changedProperties)
  if (this.shadowRoot) {
    modifyDefaultBrowserPage(this.shadowRoot)
  }
}
