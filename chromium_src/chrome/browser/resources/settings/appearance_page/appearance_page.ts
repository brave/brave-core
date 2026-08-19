// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsAppearancePageElement } from './appearance_page-chromium.js'

// Registers <settings-brave-appearance-toolbar>, which is woven into
// settings-appearance-page's template by the lit_mangler override below.
import '../brave_appearance_page/toolbar.js'

// These are hidden via CSS rather than removed from the template (in the
// companion appearance_page.html.ts lit_mangler override), because
// SettingsAppearancePageElement itself still references them: the first
// three are declared in its `$` interface, and #customize-fonts-subpage-
// trigger is used by getFocusConfig()/getAssociatedControlFor().
injectStyle(SettingsAppearancePageElement, css`
  #colorSchemeModeRow,
  .cr-row:has(#defaultFontSize),
  .cr-row:has(#pageZoom),
  #customize-fonts-subpage-trigger {
    display: none !important;
  }
`)

export * from './appearance_page-chromium.js'
