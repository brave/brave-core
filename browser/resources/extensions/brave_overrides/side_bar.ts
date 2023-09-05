// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {RegisterStyleOverride} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

RegisterStyleOverride(
  'extensions-sidebar',
  html`
    <style>
      .cr-nav-menu-item {
        min-height: 20px !important;
        border-end-end-radius: 0px !important;
        border-start-end-radius: 0px !important;
        box-sizing: content-box !important;
      }

      .cr-nav-menu-item:hover {
        background: transparent !important;
      }

      .cr-nav-menu-item[selected] {
        --iron-icon-fill-color: var(--cr-link-color) !important;
        color: var(--cr-link-color) !important;
        background: transparent !important;
      }

      .cr-nav-menu-item iron-icon {
        display: none !important;
      }

      .cr-nav-menu-item paper-ripple {
        display: none !important;
      }
    </style>
  `
)
