// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {BaseMixin} from '../base_mixin.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'

import {getTemplate} from './ad_block_only_mode_alert.html.js'

const BraveSettingsAdBlockOnlyModeAlertBase =
     PrefsMixin(BaseMixin(PolymerElement))

class BraveSettingsAdBlockOnlyModeAlert extends BraveSettingsAdBlockOnlyModeAlertBase {
  static get is() {
    return 'settings-ad-block-only-mode-alert'
  }

  static get template() {
    return getTemplate()
  }

  onTurnOffAdBlockOnlyMode_() {
    this.setPrefValue('brave.ad_block.adblock_only_mode_enabled', false)
  }
}

customElements.define(BraveSettingsAdBlockOnlyModeAlert.is, BraveSettingsAdBlockOnlyModeAlert);
