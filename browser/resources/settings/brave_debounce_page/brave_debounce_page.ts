// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {getTemplate} from './brave_debounce_page.html.js'

const SettingsBraveDebouncePageElementBase = PrefsMixin(PolymerElement)

/**
 * 'settings-brave-debounce-page' is the settings page for
 * managing debounce rules.
 */
class SettingsBraveDebouncePageElement extends SettingsBraveDebouncePageElementBase {
  static get is() {
    return 'settings-brave-debounce-page'
  }

  static get template() {
    return getTemplate()
  }
}

customElements.define(
  SettingsBraveDebouncePageElement.is,
  SettingsBraveDebouncePageElement)
