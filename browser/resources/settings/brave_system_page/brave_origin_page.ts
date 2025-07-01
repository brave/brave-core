// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './brave_origin_page.html.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BraveOriginBrowserProxy, BraveOriginBrowserProxyImpl} from './brave_origin_browser_proxy.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
/**
 * 'settings-brave-origin-page' is the settings page containing
 * Brave Origin features.
 */
const SettingsBraveOriginPageElementBase =
  PrefsMixin(BaseMixin(I18nMixin(WebUiListenerMixin(
    PolymerElement)))) as {
    new(): PolymerElement &
           PrefsMixinInterface &
           WebUiListenerMixinInterface &
           I18nMixinInterface
  }

export class SettingsBraveOriginPageElement
    extends SettingsBraveOriginPageElementBase {

  static get is() {
    return 'settings-brave-origin-page'
  }

  static get template() {
    return getTemplate()
  }

  // private toggleWireguardSubLabel_: String;
  private braveOriginEnabled_: Boolean = false;
  private toggleNtpAds_: Boolean = false;
  private toggleEmailAlias_: Boolean = false;
  private toggleLeoAi_: Boolean = false;

  private originBrowserProxy_: BraveOriginBrowserProxy =
    BraveOriginBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.originBrowserProxy_.getInitialState().then(this.onGetInitialState.bind(this))
  }

  private onGetInitialState(initial_state: any) {
    this.braveOriginEnabled_ = initial_state.enabled;
    this.toggleNtpAds_ = initial_state.toggle_ntp_ads;
    this.toggleEmailAlias_ = initial_state.toggle_email_alias;
    this.toggleLeoAi_ = initial_state.toggle_leo_ai;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-origin-page': SettingsBraveOriginPageElement
  }
}

customElements.define(
  SettingsBraveOriginPageElement.is, SettingsBraveOriginPageElement)
