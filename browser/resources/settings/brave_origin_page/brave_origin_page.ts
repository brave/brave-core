// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'

import {BraveOriginBrowserProxy, BraveOriginBrowserProxyImpl} from './brave_origin_browser_proxy.js';
import {getTemplate} from './brave_origin_page.html.js'

export interface SettingsBraveOriginPageElement {
  $: {
    toggleTorWindowsButton: SettingsToggleButtonElement,
    // TODO: implement ...
    toggleSearchAdsButton: SettingsToggleButtonElement,
    toggleEmailAliasButton: SettingsToggleButtonElement,
    toggleP3ACrashReportButton: SettingsToggleButtonElement,
    toggleSidebarButton: SettingsToggleButtonElement,
    toggleWeb3DomainsButton: SettingsToggleButtonElement
  }
}

const SettingsBraveOriginPageElementBase =
  PrefsMixin(BaseMixin(I18nMixin(WebUiListenerMixin(
    PolymerElement)))) as {
    new(): PolymerElement &
           PrefsMixinInterface &
           WebUiListenerMixinInterface &
           I18nMixinInterface
  }

/**
 * 'settings-brave-origin-page' is the settings page containing
 * Brave Origin features.
 */
export class SettingsBraveOriginPageElement
    extends SettingsBraveOriginPageElementBase {

  static get is() {
    return 'settings-brave-origin-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      braveOriginEnabled_: {
        type: Boolean,
        value: false,
      },
      // <if expr="enable_tor">
      torEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      // </if>
      showRestartToast_: Boolean,
      toggleSearchAds_: {
        type: Boolean,
        value: false
      },
      toggleEmailAlias_: {
        type: Boolean,
        value: false
      },
      toggleP3ACrashReport_: {
        type: Boolean,
        value: false
      },
      toggleSidebar_: {
        type: Boolean,
        value: false
      },
      toggleWeb3Domains_: {
        type: Boolean,
        value: false
      }
    }
  }

  private browserProxy_: BraveOriginBrowserProxy =
    BraveOriginBrowserProxyImpl.getInstance();
  declare braveOriginEnabled_: boolean
  // <if expr="enable_tor">
  declare torEnabledPref_: chrome.settingsPrivate.PrefObject
  declare torManaged_: boolean
  // </if>
  declare showRestartToast_: boolean
  declare toggleSearchAds_: boolean
  declare toggleEmailAlias_: boolean
  declare toggleLeoAi_: boolean
  declare toggleP3ACrashReport_: boolean
  declare toggleSidebar_: boolean
  declare toggleTorWindows_: boolean
  declare toggleWeb3Domains_: boolean

  override ready() {
    super.ready()
    this.browserProxy_.getInitialState().then(
      this.onGetInitialState.bind(this))
    // <if expr="enable_tor">
    this.addWebUiListener(
      'tor-enabled-changed', this.setTorEnabledPref_.bind(this))
    // </if>
    this.addWebUiListener(
      'brave-needs-restart-changed', (needsRestart: boolean) => {
        this.showRestartToast_ = needsRestart
    })
  }

  private onGetInitialState(initial_state: any) {
    this.braveOriginEnabled_ = initial_state.enabled;
    // <if expr="enable_tor">
    if (initial_state.torManaged) {
      this.torManaged_ = true;
    }
    this.setTorEnabledPref_(initial_state.tor);
    // </if>

    // TODO: implement
    this.toggleSearchAds_ = initial_state.search_ads;
    this.toggleEmailAlias_ = initial_state.toggle_email_alias;
    this.toggleP3ACrashReport_ = initial_state.toggle_p3a_crash_report;
    this.toggleSidebar_ = initial_state.toggle_sidebar;
    this.toggleWeb3Domains_ = initial_state.toggle_web3domains;
  }

  // <if expr="enable_tor">
  setTorEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      controlledBy: this.torManaged_ ?
          chrome.settingsPrivate.ControlledBy.DEVICE_POLICY :
          undefined,
      enforcement: this.torManaged_ ?
          chrome.settingsPrivate.Enforcement.ENFORCED :
          undefined,
      value: enabled,
    }
    this.torEnabledPref_ = pref
  }
  // </if>

  // <if expr="enable_tor">
  private toggleTorWindowsButtonChange_ () {
    this.browserProxy_.setTorEnabled(this.$.toggleTorWindowsButton.checked)
  }
  // </if>

  private toggleSearchAdsButtonChange_ () {
    console.log('toggleSearchAds_', this.$.toggleSearchAdsButton.checked)
  }

  private toggleEmailAliasChange_ () {
    console.log('toggleEmailAlias_', this.$.toggleEmailAliasButton.checked)
  }

  private toggleP3ACrashReportChange_ () {
    console.log('toggleP3ACrashReport_', this.$.toggleP3ACrashReportButton.checked)
  }

  private toggleSidebarChange_ () {
    console.log('toggleSidebar_', this.$.toggleSidebarButton.checked)
  }

  private toggleWeb3DomainsChange_ () {
    console.log('toggleWeb3Domains_', this.$.toggleWeb3DomainsButton.checked)
  }

  restartBrowser_(e: Event) {
    e.stopPropagation()
    window.open("chrome://restart", "_self")
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-origin-page': SettingsBraveOriginPageElement
  }
}

customElements.define(
  SettingsBraveOriginPageElement.is, SettingsBraveOriginPageElement)
