// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { ManagedFootnoteElement } from './managed_footnote-chromium.js'

// Don't show the managed footnote on the downloads page, as our downloads page
// doesn't have a static area to display the footnote and it thus interferes
// with the hit box for the "Clear All" button
if (window.location.href === 'chrome://downloads/') {
  injectStyle(ManagedFootnoteElement, css`
    :host([is-managed_]) {
      display: none !important;
    }
  `)
}

export * from './managed_footnote-chromium.js'
