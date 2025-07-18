// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {getTemplate} from './ad_block_only_mode_page.html.js'

export class BraveSettingsAdBlockOnlyModePage extends PolymerElement {
  static get is() {
    return 'settings-shields-lite-page'
  }

  static get template() {
    return getTemplate()
  }
}

customElements.define(BraveSettingsAdBlockOnlyModePage.is, BraveSettingsAdBlockOnlyModePage);
