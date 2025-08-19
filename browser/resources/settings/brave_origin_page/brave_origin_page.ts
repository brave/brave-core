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
    toggleRewardsButton: SettingsToggleButtonElement,
    toggleCrashReportingButton: SettingsToggleButtonElement,
    toggleLeoAiButton: SettingsToggleButtonElement,
    toggleNewsButton: SettingsToggleButtonElement,
    toggleP3AButton: SettingsToggleButtonElement,
    // <if expr="enable_speedreader">
    toggleSpeedreaderButton: SettingsToggleButtonElement,
    // </if>
    toggleStatsReportingButton: SettingsToggleButtonElement,
    toggleTalkButton: SettingsToggleButtonElement,
    // <if expr="enable_tor">
    toggleTorWindowsButton: SettingsToggleButtonElement,
    // </if>
    // <if expr="enable_brave_vpn">
    toggleVpnButton: SettingsToggleButtonElement,
    // </if>
    toggleWalletButton: SettingsToggleButtonElement,
    // <if expr="enable_brave_wayback_machine">
    toggleWaybackMachineButton: SettingsToggleButtonElement,
    // </if>
    toggleWebDiscoveryProjectButton: SettingsToggleButtonElement
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
      rewardsEnabledSubLabel_: {
        type: String,
        value: '',
      },
      crashReportingEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      crashReportingEnabledSubLabel_: {
        type: String,
        value: '',
      },
      crashReportingManaged_: {
        type: Boolean,
        value: false,
      },
      leoAiEnabledSubLabel_: {
        type: String,
        value: '',
      },
      newsEnabledSubLabel_ : {
        type: String,
        value: '',
      },
      p3aEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      p3aEnabledSubLabel_: {
        type: String,
        value: '',
      },
      p3aManaged_: {
        type: Boolean,
        value: false,
      },
      // <if expr="enable_speedreader">
      speedreaderEnabledSubLabel_: {
        type: String,
        value: '',
      },
      // </if>
      statsReportingEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      statsReportingEnabledSubLabel_: {
        type: String,
        value: '',
      },
      statsReportingManaged_: {
        type: Boolean,
        value: false,
      },
      talkEnabledSubLabel_: {
        type: String,
        value: '',
      },
      // <if expr="enable_tor">
      torEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      torEnabledSubLabel_: {
        type: String,
        value: '',
      },
      torManaged_: {
        type: Boolean,
        value: false,
      },
      // </if>
      // <if expr="enable_brave_vpn">
      vpnEnabledSubLabel_: {
        type: String,
        value: '',
      },
      // </if>
      walletEnabledSubLabel_: {
        type: String,
        value: '',
      },
      // <if expr="enable_brave_wayback_machine">
      waybackMachineEnabledSubLabel_: {
        type: String,
        value: '',
      },
      // </if>
      webDiscoveryProjectEnabledSubLabel_: {
        type: String,
        value: '',
      },
      showRestartToast_: Boolean
    }
  }

  private browserProxy_: BraveOriginBrowserProxy =
    BraveOriginBrowserProxyImpl.getInstance();
  declare braveOriginEnabled_: boolean
  declare rewardsEnabledSubLabel_: String
  declare crashReportingEnabledPref_: chrome.settingsPrivate.PrefObject
  declare crashReportingManaged_: boolean
  declare crashReportingEnabledSubLabel_: String
  declare leoAiEnabledSubLabel_: String
  declare newsEnabledSubLabel_: String
  declare p3aEnabledPref_: chrome.settingsPrivate.PrefObject
  declare p3aManaged_: boolean
  declare p3aEnabledSubLabel_: String
  declare statsReportingEnabledPref_: chrome.settingsPrivate.PrefObject
  declare statsReportingManaged_: boolean
  declare statsReportingEnabledSubLabel_: String
  // <if expr="enable_speedreader">
  declare speedreaderEnabledSubLabel_: String
  // </if>
  declare talkEnabledSubLabel_: String
  // <if expr="enable_tor">
  declare torEnabledPref_: chrome.settingsPrivate.PrefObject
  declare torManaged_: boolean
  declare torEnabledSubLabel_: String
  // </if>
  // <if expr="enable_brave_vpn">
  declare vpnEnabledSubLabel_: String
  // </if>
  declare walletEnabledSubLabel_: String
  // <if expr="enable_brave_wayback_machine">
  declare waybackMachineEnabledSubLabel_: String
  // </if>
  declare webDiscoveryProjectEnabledSubLabel_: String
  declare showRestartToast_: boolean

  override ready() {
    super.ready()
    this.browserProxy_.getInitialState().then(
      this.onGetInitialState.bind(this))
    this.addWebUiListener(
      'crashReporting-enabled-changed',
      this.setCrashReportingEnabledPref_.bind(this))
    this.addWebUiListener(
      'p3a-reporting-enabled-changed',
      this.setP3AReportingEnabledPref_.bind(this))
    this.addWebUiListener(
      'statsReporting-enabled-changed',
      this.setStatsReportingEnabledPref_.bind(this))
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
    if (initial_state.p3aManaged) {
      this.p3aManaged_ = true;
    }
    this.setP3AReportingEnabledPref_(initial_state.p3a);
    if (initial_state.statsReportingManaged) {
      this.statsReportingManaged_ = true;
    }
    this.setStatsReportingEnabledPref_(initial_state.statsReporting);
    if (initial_state.crashReportingManaged) {
      this.crashReportingManaged_ = true;
    }
    this.setCrashReportingEnabledPref_(initial_state.crashReporting);
    // <if expr="enable_tor">
    if (initial_state.torManaged) {
      this.torManaged_ = true;
    }
    this.setTorEnabledPref_(initial_state.tor);
    // </if>

    this.maybeShowToggleDisclaimer()
  }

  setP3AReportingEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      controlledBy: this.p3aManaged_ ?
          chrome.settingsPrivate.ControlledBy.DEVICE_POLICY :
          undefined,
      enforcement: this.p3aManaged_ ?
          chrome.settingsPrivate.Enforcement.ENFORCED :
          undefined,
      value: enabled,
    }
    this.p3aEnabledPref_ = pref
  }

  setStatsReportingEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      controlledBy: this.statsReportingManaged_ ?
          chrome.settingsPrivate.ControlledBy.DEVICE_POLICY :
          undefined,
      enforcement: this.statsReportingManaged_ ?
          chrome.settingsPrivate.Enforcement.ENFORCED :
          undefined,
      value: enabled,
    }
    this.statsReportingEnabledPref_ = pref
  }

  setCrashReportingEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      controlledBy: this.crashReportingManaged_ ?
          chrome.settingsPrivate.ControlledBy.DEVICE_POLICY :
          undefined,
      enforcement: this.crashReportingManaged_ ?
          chrome.settingsPrivate.Enforcement.ENFORCED :
          undefined,
      value: enabled,
    }
    this.crashReportingEnabledPref_ = pref
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

  private maybeShowToggleDisclaimer () {
    const enabledText = this.i18n('braveOriginItemEnabledSubtitle')

    this.rewardsEnabledSubLabel_ = this.$.toggleRewardsButton.checked ?
        enabledText : ''
    this.crashReportingEnabledSubLabel_ =
        this.$.toggleCrashReportingButton.checked ?
        enabledText : ''
    this.leoAiEnabledSubLabel_ = this.$.toggleLeoAiButton.checked ?
        enabledText : ''
    this.newsEnabledSubLabel_ = this.$.toggleNewsButton.checked ?
        enabledText : ''
    this.p3aEnabledSubLabel_ = this.$.toggleP3AButton.checked ?
        enabledText : ''
    // <if expr="enable_speedreader">
    this.speedreaderEnabledSubLabel_ = this.$.toggleSpeedreaderButton.checked ?
        enabledText : ''
    // </if>
    this.statsReportingEnabledSubLabel_ =
        this.$.toggleStatsReportingButton.checked ?
        enabledText : ''
    this.talkEnabledSubLabel_ = this.$.toggleTalkButton.checked ?
        enabledText : ''
    // <if expr="enable_tor">
    this.torEnabledSubLabel_ = this.$.toggleTorWindowsButton.checked ?
        enabledText : ''
    // </if>
    // <if expr="enable_brave_vpn">
    this.vpnEnabledSubLabel_ = this.$.toggleVpnButton.checked ?
        enabledText : ''
    // </if>
    this.walletEnabledSubLabel_ = this.$.toggleWalletButton.checked ?
        enabledText : ''
    // <if expr="enable_brave_wayback_machine">
    this.waybackMachineEnabledSubLabel_ =
        this.$.toggleWaybackMachineButton.checked ?
        enabledText : ''
    // </if>
    this.webDiscoveryProjectEnabledSubLabel_ =
        this.$.toggleWebDiscoveryProjectButton.checked ?
        enabledText : ''
  }

  private toggleRewardsButtonChange_ () {
    this.maybeShowToggleDisclaimer()
  }

  private toggleCrashReportingButtonChange_ () {
    this.browserProxy_.setCrashReportingEnabled(
        this.$.toggleCrashReportingButton.checked)
    this.maybeShowToggleDisclaimer()
  }

  private toggleButtonChange_ () {
    this.maybeShowToggleDisclaimer()
  }

  private toggleP3AButtonChange_ () {
    this.browserProxy_.setP3AEnabled(
        this.$.toggleP3AButton.checked)
    this.maybeShowToggleDisclaimer()
  }

  private toggleStatsReportingButtonChange_ () {
    this.browserProxy_.setStatsReportingEnabled(
        this.$.toggleStatsReportingButton.checked)
    this.maybeShowToggleDisclaimer()
  }

  // <if expr="enable_tor">
  private toggleTorWindowsButtonChange_ () {
    this.browserProxy_.setTorEnabled(this.$.toggleTorWindowsButton.checked)
    this.maybeShowToggleDisclaimer()
  }
  // </if>

  private resetToDefaults_ () {
    this.browserProxy_.resetToDefaults()
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
