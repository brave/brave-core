// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'

import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'

import {getTemplate} from './ad_block_only_mode_page.html.js'

import {
  DefaultBraveShieldsBrowserProxy,
  DefaultBraveShieldsBrowserProxyImpl
} from '../default_brave_shields_page/default_brave_shields_browser_proxy.js'

interface BraveSettingsAdBlockOnlyModePage {
  $: {
    adBlockOnlyModeEmbedControlType: SettingsToggleButtonElement,
  }
}

const BraveSettingsAdBlockOnlyModePageBase =
  WebUiListenerMixin(I18nMixin(PrefsMixin(PolymerElement)))

class BraveSettingsAdBlockOnlyModePage extends BraveSettingsAdBlockOnlyModePageBase {
  static get is() {
    return 'settings-ad-block-only-mode-page'
  }

  static get template() {
    return getTemplate()
  }

  private browserProxy_: DefaultBraveShieldsBrowserProxy =
    DefaultBraveShieldsBrowserProxyImpl.getInstance()

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

  private declare isAdBlockOnlyModeEnabled_: chrome.settingsPrivate.PrefObject<boolean>

  override ready() {
    super.ready();

    this.onShieldsSettingsChanged_();

    this.addWebUiListener('brave-shields-settings-changed',
      () => { this.onShieldsSettingsChanged_() });
  }

  onShieldsSettingsChanged_ () {
    this.browserProxy_.getAdBlockOnlyModeEnabled().then(value => {
      this.isAdBlockOnlyModeEnabled_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: value,
      }
    });
  }

  onAdBlockOnlyModeChange_() {
    this.browserProxy_.setAdBlockOnlyModeEnabled(
      this.$.adBlockOnlyModeEmbedControlType.checked
    );
  }
}

customElements.define(BraveSettingsAdBlockOnlyModePage.is, BraveSettingsAdBlockOnlyModePage);
