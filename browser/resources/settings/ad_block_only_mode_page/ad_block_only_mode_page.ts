// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'

import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'

import {getTemplate} from './ad_block_only_mode_page.html.js'

import {
  AdBlockOnlyModeBrowserProxy,
  AdBlockOnlyModeBrowserProxyImpl
} from './ad_block_only_mode_browser_proxy.js'

interface BraveSettingsAdBlockOnlyModePage {
  $: {
    adBlockOnlyModeEmbedControlType: SettingsToggleButtonElement,
  }
}

const BraveSettingsAdBlockOnlyModePageBase =
  WebUiListenerMixin(PrefsMixin(PolymerElement))

class BraveSettingsAdBlockOnlyModePage
    extends BraveSettingsAdBlockOnlyModePageBase {
  static get is() {
    return 'settings-ad-block-only-mode-page'
  }

  static get template() {
    return getTemplate()
  }

  private browserProxy_: AdBlockOnlyModeBrowserProxy =
    AdBlockOnlyModeBrowserProxyImpl.getInstance()

  static get properties () {
    return {
      isAdBlockOnlyModeEnabled_: {
        type: Object,
        value: {
          key: '',
          type: chrome.settingsPrivate.PrefType.BOOLEAN,
          value: false,
        }
      }
    }
  }

  private declare isAdBlockOnlyModeEnabled_:
      chrome.settingsPrivate.PrefObject<boolean>

  override ready() {
    super.ready();

    this.onAdBlockOnlyModeChanged_();

    this.addWebUiListener('ad-block-only-mode-enabled-changed',
      () => { this.onAdBlockOnlyModeChanged_() });
  }

  onAdBlockOnlyModeChanged_() {
    this.browserProxy_.getAdBlockOnlyModeEnabled().then(value => {
      this.isAdBlockOnlyModeEnabled_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: value,
      }
    });
  }

  onAdBlockOnlyModeSettingChanged_() {
    this.browserProxy_.setAdBlockOnlyModeEnabled(
      this.$.adBlockOnlyModeEmbedControlType.checked
    );
  }
}

customElements.define(BraveSettingsAdBlockOnlyModePage.is,
                      BraveSettingsAdBlockOnlyModePage);
