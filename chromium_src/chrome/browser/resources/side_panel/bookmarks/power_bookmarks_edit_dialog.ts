// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { RegisterStyleOverride } from '//resources/brave/polymer_overriding.js'
import { html } from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js'

export * from './power_bookmarks_edit_dialog-chromium.js'

RegisterStyleOverride('power-bookmarks-edit-dialog', html`<style>
  :host {
    --leo-button-padding: 4px;
  }
</style>`)
