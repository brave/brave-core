// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {HistoryItemElement} from './history_item-chromium.js'

injectStyle(HistoryItemElement, css`
  .website-title {
    font-size: 13px;
    font-weight: 400;
  }

  #menu-button {
    transform: rotate(90deg) !important;
  }
`)

export * from './history_item-chromium.js'
