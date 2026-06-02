// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html
} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { RegisterStyleOverride } from '//resources/brave/polymer_overriding.js'

RegisterStyleOverride('power-bookmark-row', html`<style>
  :host {
    --iron-icon-width: 16px;
  }
</style>`)

export * from './power_bookmark_row-chromium.js'
