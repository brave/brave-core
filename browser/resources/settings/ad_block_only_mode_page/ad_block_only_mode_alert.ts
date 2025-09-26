// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {loadTimeData} from '../i18n_setup.js'

import {getTemplate} from './ad_block_only_mode_alert.html.js'

class BraveSettingsAdBlockOnlyModeAlert extends PrefsMixin(PolymerElement) {
  static readonly AD_BLOCK_ONLY_MODE_ENABLED_PREF =
      'brave.shields.adblock_only_mode_enabled'

  static get is() {
    return 'settings-ad-block-only-mode-alert'
  }

  static get template() {
    return getTemplate()
  }

  static get properties () {
    return {
      isAdBlockOnlyModeEnabled_: {
        type: Boolean,
        value: false,
      },
    }
  }

  private declare isAdBlockOnlyModeEnabled_: boolean

  override ready() {
    super.ready();

    chrome.settingsPrivate.getPref(
      BraveSettingsAdBlockOnlyModeAlert.AD_BLOCK_ONLY_MODE_ENABLED_PREF)
      .then((pref: chrome.settingsPrivate.PrefObject) =>
        this.onAdBlockOnlyModeEnabledUpdated_(pref));

    chrome.settingsPrivate.onPrefsChanged.addListener(
      (prefs: chrome.settingsPrivate.PrefObject[]) => {
        const pref = prefs.find(
          p => p.key === BraveSettingsAdBlockOnlyModeAlert.AD_BLOCK_ONLY_MODE_ENABLED_PREF
        )
        if (pref) {
          this.onAdBlockOnlyModeEnabledUpdated_(pref)
        }
      });
  }

  private onAdBlockOnlyModeEnabledUpdated_(pref: chrome.settingsPrivate.PrefObject): void {
    this.isAdBlockOnlyModeEnabled_ =
        pref.value &&
        loadTimeData.getBoolean('isAdBlockOnlyModeFeatureEnabled')
  }

  onTurnOffAdBlockOnlyMode_() {
    this.setPrefValue(
        BraveSettingsAdBlockOnlyModeAlert.AD_BLOCK_ONLY_MODE_ENABLED_PREF,
        false)
  }
}

customElements.define(BraveSettingsAdBlockOnlyModeAlert.is,
                      BraveSettingsAdBlockOnlyModeAlert);
