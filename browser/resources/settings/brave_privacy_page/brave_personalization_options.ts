// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {SettingsToggleButtonElement} from '/shared/settings/controls/settings_toggle_button.js';
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {BaseMixin} from '../base_mixin.js'
import {loadTimeData} from '../i18n_setup.js';

import {getTemplate} from './brave_personalization_options.html.js'
import {BravePrivacyBrowserProxy, BravePrivacyBrowserProxyImpl} from './brave_privacy_page_browser_proxy.js'

const SettingsBravePersonalizationOptionsBase =
  WebUiListenerMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & WebUiListenerMixinInterface
  }

export interface SettingsBravePersonalizationOptions {
  $: {
    p3aEnabled: SettingsToggleButtonElement,
    statsUsagePingEnabled: SettingsToggleButtonElement,
  }
}

export class SettingsBravePersonalizationOptions extends SettingsBravePersonalizationOptionsBase {
  static get is() {
    return 'settings-brave-personalization-options'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      webRTCPolicies_: {
        readOnly: true,
        type: Array,
        value: function () {
          return [
            { value: 'default', name: loadTimeData.getString('webRTCDefault') },
            { value: 'default_public_and_private_interfaces', name: loadTimeData.getString('defaultPublicAndPrivateInterfaces') },
            { value: 'default_public_interface_only', name: loadTimeData.getString('defaultPublicInterfaceOnly') },
            { value: 'disable_non_proxied_udp', name: loadTimeData.getString('disableNonProxiedUdp') }
          ]
        },
      },
      webRTCPolicy_: String,
      p3aEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {};
        },
      },
      statsUsagePingEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {};
        },
      },
      isRequestOTRFeatureEnabled_: {
        readOnly: true,
        type: Boolean,
        value: function () {
          return loadTimeData.getBoolean('isRequestOTRFeatureEnabled')
        }
      },
      requestOTRActions_: {
        readOnly: true,
        type: Array,
        value: function () {
          return [
            { value: 0, name: loadTimeData.getString('requestOTRDefault') },
            { value: 1, name: loadTimeData.getString('requestOTRAlways') },
            { value: 2, name: loadTimeData.getString('requestOTRNever') },
          ]
        },
      },
      requestOTRAction_: String,
    };
  }

  private webRTCPolicies_: Object[];
  private webRTCPolicy_: String;
  private p3aEnabledPref_: Object;
  private statsUsagePingEnabledPref_: Object;
  private requestOTRActions_: Object[];
  private requestOTRAction_: String;

  browserProxy_: BravePrivacyBrowserProxy = BravePrivacyBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    // Used for first time initialization of checked state.
    // Can't use `prefs` property of `settings-toggle-button` directly
    // because p3a enabled is a local state setting, but PrefControlMixin
    // checks for a pref being valid, so have to fake it, same as upstream.
    const setP3AEnabledPref = (enabled: boolean) => this.setP3AEnabledPref_(enabled);
    this.addWebUiListener('p3a-enabled-changed', setP3AEnabledPref);
    this.browserProxy_.getP3AEnabled().then(
      (enabled: boolean) => setP3AEnabledPref(enabled));

    const setStatsUsagePingEnabledPref = (enabled: boolean) => this.setStatsUsagePingEnabledPref_(enabled);
    this.addWebUiListener(
      'stats-usage-ping-enabled-changed', setStatsUsagePingEnabledPref);
    this.browserProxy_.getStatsUsagePingEnabled().then(
      (enabled: boolean) => setStatsUsagePingEnabledPref(enabled));
  }

  setP3AEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.p3aEnabledPref_ = pref;
  }

  onP3AEnabledChange_() {
    this.browserProxy_.setP3AEnabled(this.$.p3aEnabled.checked);
  }

  setStatsUsagePingEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.statsUsagePingEnabledPref_ = pref;
  }

  onStatsUsagePingEnabledChange_() {
    this.browserProxy_.setStatsUsagePingEnabled(this.$.statsUsagePingEnabled.checked);
  }

  shouldShowRestart_(enabled: boolean) {
    return enabled != this.browserProxy_.wasPushMessagingEnabledAtStartup();
  }

  restartBrowser_(e: Event) {
    e.stopPropagation();
    window.open("chrome://restart", "_self");
  }
}

customElements.define(
  SettingsBravePersonalizationOptions.is, SettingsBravePersonalizationOptions)
