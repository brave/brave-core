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
    toggleP3AButton: SettingsToggleButtonElement,
    toggleStatsReportingButton: SettingsToggleButtonElement,
    toggleCrashReportingButton: SettingsToggleButtonElement,
    toggleTorWindowsButton: SettingsToggleButtonElement
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
      p3aEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      statsReportingEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      crashReportingEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      // <if expr="enable_tor">
      torEnabledPref_: {
        type: Object,
        value() { return {} }
      },
      // </if>
      showRestartToast_: Boolean
    }
  }

  private browserProxy_: BraveOriginBrowserProxy =
    BraveOriginBrowserProxyImpl.getInstance();
  declare braveOriginEnabled_: boolean
  declare showRestartToast_: boolean
  declare p3aEnabledPref_: chrome.settingsPrivate.PrefObject
  declare p3aManaged_: boolean
  declare statsReportingEnabledPref_: chrome.settingsPrivate.PrefObject
  declare statsReportingManaged_: boolean
  declare crashReportingEnabledPref_: chrome.settingsPrivate.PrefObject
  declare crashReportingManaged_: boolean
  // <if expr="enable_tor">
  declare torEnabledPref_: chrome.settingsPrivate.PrefObject
  declare torManaged_: boolean
  // </if>

  override ready() {
    super.ready()
    this.browserProxy_.getInitialState().then(
      this.onGetInitialState.bind(this))
    this.addWebUiListener(
      'p3a-reporting-enabled-changed',
      this.setP3AReportingEnabledPref_.bind(this))
    this.addWebUiListener(
      'statsReporting-enabled-changed',
      this.setStatsReportingEnabledPref_.bind(this))
    this.addWebUiListener(
      'crashReporting-enabled-changed',
      this.setCrashReportingEnabledPref_.bind(this))
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

  private toggleP3AButtonChange_ () {
    this.browserProxy_.setP3AEnabled(
        this.$.toggleP3AButton.checked)
  }

  private toggleStatsReportingButtonChange_ () {
    this.browserProxy_.setStatsReportingEnabled(
        this.$.toggleStatsReportingButton.checked)
  }

  private toggleCrashReportingButtonChange_ () {
    this.browserProxy_.setCrashReportingEnabled(
        this.$.toggleCrashReportingButton.checked)
  }

  // <if expr="enable_tor">
  private toggleTorWindowsButtonChange_ () {
    this.browserProxy_.setTorEnabled(this.$.toggleTorWindowsButton.checked)
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
