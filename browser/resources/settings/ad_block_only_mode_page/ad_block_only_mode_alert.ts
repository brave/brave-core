// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {AdBlockOnlyModeMixin} from './ad_block_only_mode_mixin.js'

import {getTemplate} from './ad_block_only_mode_alert.html.js'

class BraveSettingsAdBlockOnlyModeAlert extends AdBlockOnlyModeMixin(PolymerElement) {
  static get is() {
    return 'settings-ad-block-only-mode-alert'
  }

  static get template() {
    return getTemplate()
  }

  onTurnOffAdBlockOnlyMode_() {
    this.setAdBlockOnlyModeEnabled(false)
  }
}

customElements.define(BraveSettingsAdBlockOnlyModeAlert.is,
                      BraveSettingsAdBlockOnlyModeAlert);
