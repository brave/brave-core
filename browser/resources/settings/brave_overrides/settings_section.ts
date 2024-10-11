// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterStyleOverride(
  'settings-section',
  html`
    <style include="settings-shared">
      #header .title {
        font-weight: 500 !important;
      }

      :host(:not(.expanded)) #card {
        box-shadow: 0px var(--Elevation-xxs, 1px) 0px 0px var(--Semantic-Elevation-Primary, rgba(0, 0, 0, 0.05)), 0px var(--Elevation-xxs, 1px) var(--Elevation-xs, 4px) 0px var(--Semantic-Elevation-Secondary, rgba(0, 0, 0, 0.10)) !important;
      }
    </style>
  `
)
