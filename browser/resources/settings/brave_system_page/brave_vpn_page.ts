// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_components/settings_prefs/prefs.js';
import '../relaunch_confirmation_dialog.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin, PrefsMixinInterface} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {loadTimeData} from "../i18n_setup.js"
import {getTemplate} from './brave_vpn_page.html.js'
import {RelaunchMixin, RelaunchMixinInterface, RestartType} from '../relaunch_mixin.js'

/**
 * 'settings-brave-vpn-page' is the settings page containing
 * brave's vpn features.
 */
const SettingsBraveVpnPageElementBase =
  PrefsMixin(BaseMixin(RelaunchMixin(PolymerElement))) as {
    new(): PolymerElement & PrefsMixinInterface & RelaunchMixinInterface
  }

export class SettingsBraveVpnPageElement
    extends SettingsBraveVpnPageElementBase {

  static get is() {
    return 'settings-brave-vpn-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return  {
      prefs: {
        type: Object,
        notify: true,
      }
    }
  }

  private initialProtocolValue_: boolean;

  override ready() {
    super.ready();
    this.initialProtocolValue_ =
      this.getPref('brave.brave_vpn.wireguard_enabled').value
  }

  private showVpnPage_(): boolean {
    return loadTimeData.getBoolean('isBraveVPNEnabled')
  }

  private onRestartClick_(e: Event) {
    // Prevent event from bubbling up to the toggle button.
    e.stopPropagation();
    this.performRestart(RestartType.RESTART);
  }

  private shouldShowRestart_(enabled: boolean): boolean {
    return this.initialProtocolValue_ !== enabled;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-vpn-page': SettingsBraveVpnPageElement
  }
}

customElements.define(
  SettingsBraveVpnPageElement.is, SettingsBraveVpnPageElement)
