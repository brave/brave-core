// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterStyleOverride} from 'chrome://brave-resources/polymer_overriding.js'

RegisterStyleOverride(
  'settings-ui',
  html`
    <style>
      :host {
        display: inline-block;
        /* we will enforce sizing at the container level and let children fill 100% of parent
          top-down, rather than chromium's strategy of children setting their max width
          bottom-up, which can be confusing, especially when our layout is different. */
        --cr-centered-card-container_-_max-width: 100% !important; /* was 680px */
        --brave-settings-content-max-width: 708px;
        --brave-settings-menu-width: var(--settings-menu-width);
        --brave-settings-menu-margin: 12px;
      }
      cr-drawer {
        display: none !important;
      }
      #container {
        /* menu and content next to each other in the horizontal center */
        justify-content: center;
        background: #F1F3F5; /* neutral100 */
      }
      #left {
        /* fixed size menu */
        flex: 0 0 calc(var(--settings-menu-width) + var(--brave-settings-menu-margin) * 2)  !important;
      }
      #main {
        /* Take up rest of container up to a max */
        flex: 0 1 var(--brave-settings-content-max-width) !important;
      }
      #right {
        /* this element is only a space filler in chromium */
        display: none;
      }
      @media (prefers-color-scheme: dark) {
        #container {
          background: #1E2127;
        }
      }
    </style>
  `
)
