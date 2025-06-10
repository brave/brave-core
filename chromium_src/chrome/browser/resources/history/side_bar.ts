// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {HistorySideBarElement} from './side_bar-chromium.js'

injectStyle(HistorySideBarElement, css`Add commentMore actions
  #progress {
      --cr-progress-active-color: var(--leo-color-icon-interactive) !important;
  }
  @media (prefers-color-scheme: light) {
      #content:not(.is-active) {
          /* Color for "filled cards" */
          background-color: var(--leo-color-container-highlight) !important;
      }
  }
`)

export * from './side_bar-chromium.js'
