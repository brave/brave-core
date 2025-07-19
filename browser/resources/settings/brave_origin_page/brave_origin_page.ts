// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '/shared/settings/prefs/prefs.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'
import {BraveOriginBrowserProxy, BraveOriginBrowserProxyImpl} from './brave_origin_browser_proxy.js';
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './brave_origin_page.html.js'

export interface SettingsBraveOriginPageElement {
  $: {
    toggleRewardsButton: SettingsToggleButtonElement,
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
      rewardsEnabledPref_: {
        type: Object,
        value() {
           return {}
        }
      },
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

  private declare braveOriginEnabled_: boolean
  private declare rewardsEnabledPref_: chrome.settingsPrivate.PrefObject
  private declare toggleSearchAds_: boolean
  private declare toggleEmailAlias_: boolean
  private declare toggleLeoAi_: boolean
  private declare toggleP3ACrashReport_: boolean
  private declare toggleSidebar_: boolean
  private declare toggleTorWindows_: boolean
  private declare toggleWeb3Domains_: boolean
  declare showRestartToast_: boolean

  private originBrowserProxy_: BraveOriginBrowserProxy =
    BraveOriginBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.originBrowserProxy_.getInitialState().then(
      this.onGetInitialState.bind(this)
    )
  }

  private onGetInitialState(initial_state: any) {
    console.log(initial_state)
    this.braveOriginEnabled_ = initial_state.enabled;
    this.toggleSearchAds_ = initial_state.search_ads;
    this.toggleEmailAlias_ = initial_state.toggle_email_alias;
    this.toggleP3ACrashReport_ = initial_state.toggle_p3a_crash_report;
    this.toggleSidebar_ = initial_state.toggle_sidebar;
    this.toggleWeb3Domains_ = initial_state.toggle_web3domains;

    this.setRewardsEnabledPref_(initial_state.rewards_enabled)
  }

  setRewardsEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.rewardsEnabledPref_ = pref
  }

  private toggleRewardsButtonChange_ () {
    this.originBrowserProxy_.setRewardsEnabled(this.$.toggleRewardsButton.checked)
  }

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
