/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'

import { SettingsSafetyHubPageElement } from '../safety_hub/safety_hub_page.js'

// Brave doesn't show the passwords card. Upstream only colors the `cr:check`
// header icon, so color the `shield-done-filled` one the
// safety_hub_page.html.ts.lit_mangler.ts override swaps in.
injectStyle(SettingsSafetyHubPageElement, css`
  #passwords {
    display: none;
  }

  #emptyStateModule {
    --iron-icon-fill-color: var(--google-green-700);
  }
`)
