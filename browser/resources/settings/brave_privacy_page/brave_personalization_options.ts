/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerMixin, WebUIListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {BravePrivacyBrowserProxy, BravePrivacyBrowserProxyImpl} from './brave_privacy_page_browser_proxy.js'
import {loadTimeData} from '../i18n_setup.js';
import {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js';
import {getTemplate} from './brave_personalization_options.html.js'

const SettingsBravePersonalizationOptionsBase =
  WebUIListenerMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & WebUIListenerMixinInterface
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
    };
  }

  private webRTCPolicies_: Object[];
  private webRTCPolicy_: String;
  private p3aEnabledPref_: Object;
  private statsUsagePingEnabledPref_: Object;

  browserProxy_: BravePrivacyBrowserProxy = BravePrivacyBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    // Used for first time initialization of checked state.
    // Can't use `prefs` property of `settings-toggle-button` directly
    // because p3a enabled is a local state setting, but PrefControlMixin
    // checks for a pref being valid, so have to fake it, same as upstream.
    const setP3AEnabledPref = (enabled: boolean) => this.setP3AEnabledPref_(enabled);
    this.addWebUIListener('p3a-enabled-changed', setP3AEnabledPref);
    this.browserProxy_.getP3AEnabled().then(
      (enabled: boolean) => setP3AEnabledPref(enabled));

    const setStatsUsagePingEnabledPref = (enabled: boolean) => this.setStatsUsagePingEnabledPref_(enabled);
    this.addWebUIListener('stats-usage-ping-enabled-changed', setStatsUsagePingEnabledPref);
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
