// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {RegisterStyleOverride} from 'chrome://brave-resources/polymer_overriding.js'

RegisterStyleOverride(
  'bookmarks-folder-node',
  html`
    <style>
<if expr="not is_macosx">
      @media (prefers-color-scheme: dark) {
        .folder-icon[open] { content: url(chrome://theme/IDR_BRAVE_BOOKMARK_FOLDER_OPEN_WHITE); }
        .folder-icon { content: url(chrome://theme/IDR_BRAVE_BOOKMARK_FOLDER_CLOSED_WHITE); }
      }
</if>
    </style>
  `
)
