// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './brave_vpn_page.html.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BraveVPNBrowserProxy, BraveVPNBrowserProxyImpl} from './brave_vpn_browser_proxy.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'

export interface SettingsBraveVpnPageElement {
  $: {
    // <if expr="is_win">
    toggleShowInSystemTray: SettingsToggleButtonElement
    // </if>
  }
}

const SettingsBraveVpnPageElementBase =
  PrefsMixin(BaseMixin(I18nMixin(WebUiListenerMixin(
    PolymerElement)))) as new() => PolymerElement &
           PrefsMixinInterface &
           WebUiListenerMixinInterface &
           I18nMixinInterface

/**
 * 'settings-brave-vpn-page' is the settings page containing
 * brave's vpn features.
 */
export class SettingsBraveVpnPageElement
    extends SettingsBraveVpnPageElementBase {

  static get is() {
    return 'settings-brave-vpn-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      // <if expr="is_win">
      showInSystemTrayPref_: {
        type: Object,
        value() {
          return {}
        }
      }
      // </if>
    }
  }

  private toggleWireguardSubLabel_: String;
  private braveVpnConnected_: Boolean = false;
  // <if expr="is_win">
  declare showInSystemTrayPref_: chrome.settingsPrivate.PrefObject;
  // </if>
  private vpnBrowserProxy_: BraveVPNBrowserProxy =
    BraveVPNBrowserProxyImpl.getInstance();

  override ready() {
    super.ready();
    this.addWebUiListener('brave-vpn-state-change',
      this.onVpnStateChange.bind(this))
    // <if expr="is_win">
    this.vpnBrowserProxy_.isBraveVpnConnected()
      .then(this.onVpnStateChange.bind(this))
    const setShowInSystemTrayPref = (enabled: boolean) =>
        this.setShowInSystemTrayPref_(enabled)
    this.vpnBrowserProxy_.showInSystemTray().then(setShowInSystemTrayPref)
    // </if>
  }

  private onVpnStateChange(connected: boolean) {
    this.braveVpnConnected_ = connected
    this.updateState()
  }

  // <if expr="is_win">
  setShowInSystemTrayPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.showInSystemTrayPref_ = pref
  }

  onToggleShowInSystemTrayChange_ () {
    this.vpnBrowserProxy_.
      setShowInSystemTray(this.$.toggleShowInSystemTray.checked)
  }
  // </if>

  private updateState() {
    this.toggleWireguardSubLabel_ = this.braveVpnConnected_ ?
      this.i18n('sublabelVpnConnected') : ''
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-vpn-page': SettingsBraveVpnPageElement
  }
}

customElements.define(
  SettingsBraveVpnPageElement.is, SettingsBraveVpnPageElement)
