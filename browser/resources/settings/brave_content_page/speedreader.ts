// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUiListenerMixin } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import { PrefsMixin } from '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { getTemplate } from './speedreader.html.js'

const SpeedreaderBase = WebUiListenerMixin(
  I18nMixin(PrefsMixin(PolymerElement))
)

/**
 * 'settings-content-speedreader' is the settings page containing settings for Speedreader
 */
class SettingsBraveContentSpeedreaderElement extends SpeedreaderBase {
  static get is () {
    return 'settings-brave-content-speedreader'
  }

  static get template () {
    return getTemplate()
  }
}

customElements.define(SettingsBraveContentSpeedreaderElement.is, SettingsBraveContentSpeedreaderElement)
