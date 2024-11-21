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
        font-size: var(--leo-typography-heading-h4-font-size) !important;
        font-weight: 600 !important;
        padding-top: var(--leo-spacing-xl) !important;
        padding-bottom: var(--leo-spacing-xl) !important;
        margin-bottom: 0 !important;
        letter-spacing: 0 !important;
      }
      :host(:not(.expanded)) #card {
        box-shadow: var(--leo-effect-elevation-01) !important;
        background-color: var(--leo-color-container-background) !important;
        border-radius: var(--leo-radius-m) !important;
      }
      .cr-row {
        border-top: var(--leo-color-divider-subtle) !important;
      }
      .cr-row:first-child {
        border-top: none !important;
      }
      .cr-centered-card-container {
        height: auto !important;
      }
      </style>
  `
)
