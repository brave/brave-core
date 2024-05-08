/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'

import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BraveDataCollectionBrowserProxyImpl} from './brave_data_collection_browser_proxy.js'
import {getTemplate} from './brave_data_collection_page.html.js'

import {MetricsReporting} from '/shared/settings/privacy_page/privacy_page_browser_proxy.js'

const SettingBraveDataCollectionPageElementBase =
  WebUiListenerMixin(PrefsMixin(PolymerElement))

interface SettingsBraveDataCollectionPageElement {
  $: {
    p3aEnabled: SettingsToggleButtonElement,
    statsUsagePingEnabled: SettingsToggleButtonElement,
    metricsReportingControl: SettingsToggleButtonElement,
  }
}

/**
 * 'settings-brave-data-collection-page' is the settings page containing
 * Brave's data collection settings.
 */
class SettingsBraveDataCollectionPageElement
extends SettingBraveDataCollectionPageElementBase
{
  static get is() {
    return 'settings-brave-data-collection-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      p3aEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      },
      statsUsagePingEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      },
      metricsReportingPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      },
      showRestartForMetricsReporting_: Boolean,
    }
  }

  private p3aEnabledPref_: Object
  private statsUsagePingEnabledPref_: Object
  private metricsReportingPref_: chrome.settingsPrivate.PrefObject<boolean>
  private showRestartForMetricsReporting_: boolean

  browserProxy_ = BraveDataCollectionBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    // Used for first time initialization of checked state.
    // Can't use `prefs` property of `settings-toggle-button` directly
    // because p3a enabled is a local state setting, but PrefControlMixin
    // checks for a pref being valid, so have to fake it, same as upstream.
    const setP3AEnabledPref = (enabled: boolean) =>
      this.setP3AEnabledPref_(enabled)
    this.addWebUiListener('p3a-enabled-changed', setP3AEnabledPref)
    this.browserProxy_.getP3AEnabled().then(
      (enabled: boolean) => setP3AEnabledPref(enabled))

    const setMetricsReportingPref = (metricsReporting: MetricsReporting) =>
        this.setMetricsReportingPref_(metricsReporting)
    this.addWebUiListener('metrics-reporting-change', setMetricsReportingPref)
    this.browserProxy_.getMetricsReporting().then(setMetricsReportingPref)

    const setStatsUsagePingEnabledPref = (enabled: boolean) =>
      this.setStatsUsagePingEnabledPref_(enabled)
    this.addWebUiListener(
      'stats-usage-ping-enabled-changed', setStatsUsagePingEnabledPref)
    this.browserProxy_.getStatsUsagePingEnabled().then(
      (enabled: boolean) => setStatsUsagePingEnabledPref(enabled))
  }

  setP3AEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.p3aEnabledPref_ = pref
  }

  onP3AEnabledChange_() {
    this.browserProxy_.setP3AEnabled(this.$.p3aEnabled.checked)
  }

  setStatsUsagePingEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.statsUsagePingEnabledPref_ = pref
  }

  onStatsUsagePingEnabledChange_() {
    this.browserProxy_.
      setStatsUsagePingEnabled(this.$.statsUsagePingEnabled.checked)
  }

  // Metrics related code is copied from
  // chrome/browser/resources/settings/privacy_page/personalization_options.ts
  // as we hide that upstream option and put it in this page.
  onMetricsReportingChange_() {
    const enabled = this.$.metricsReportingControl.checked
    this.browserProxy_.setMetricsReportingEnabled(enabled)
  }

  setMetricsReportingPref_(metricsReporting: MetricsReporting) {
    const hadPreviousPref = this.metricsReportingPref_.value !== undefined
    const pref: chrome.settingsPrivate.PrefObject<boolean> = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: metricsReporting.enabled,
    }
    if (metricsReporting.managed) {
      pref.enforcement = chrome.settingsPrivate.Enforcement.ENFORCED
      pref.controlledBy = chrome.settingsPrivate.ControlledBy.USER_POLICY
    }

    // Ignore the next change because it will happen when we set the pref.
    this.metricsReportingPref_ = pref

    // TODO(dbeam): remember whether metrics reporting was enabled when Chrome
    // started.
    if (metricsReporting.managed) {
      this.showRestartForMetricsReporting_ = false
    } else if (hadPreviousPref) {
      this.showRestartForMetricsReporting_ = true
    }
  }

  restartBrowser_(e: Event) {
    e.stopPropagation()
    window.open("chrome://restart", "_self")
  }
}

customElements.define(
  SettingsBraveDataCollectionPageElement.is,
  SettingsBraveDataCollectionPageElement)
