// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'

import { PowerBookmarksEditDialogElement } from './power_bookmarks_edit_dialog-chromium.js'

injectStyle(PowerBookmarksEditDialogElement, css`
  :host {
    --leo-button-padding: 4px;
  }
`)

export * from './power_bookmarks_edit_dialog-chromium.js'
