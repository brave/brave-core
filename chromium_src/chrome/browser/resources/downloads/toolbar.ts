// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { DownloadsToolbarElement } from './toolbar-chromium.js'

// Our downloads page doesn't have a static area to display the "Clear All"
// button, so it ends up overlapping the scrollbar when the list of download
// items is long. Add some margin to the button to avoid that.
injectStyle(DownloadsToolbarElement, css`
  #clearAll {
    margin-right: 20px;
  }
`)

export * from './toolbar-chromium.js'
