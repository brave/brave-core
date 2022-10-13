/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { Polymer } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUIListenerBehavior} from 'chrome://resources/cr_elements/web_ui_listener_behavior.js';
import {BravePrivacyBrowserProxy, BravePrivacyBrowserProxyImpl} from './brave_privacy_page_browser_proxy.js'

Polymer({
  is: 'settings-brave-personalization-options',

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    webRTCPolicies_: {
      readOnly: true,
      type: Array,
      value: function() {
        return [
          {value: 'default', name: loadTimeData.getString('webRTCDefault')},
          {value: 'default_public_and_private_interfaces', name: loadTimeData.getString('defaultPublicAndPrivateInterfaces')},
          {value: 'default_public_interface_only', name: loadTimeData.getString('defaultPublicInterfaceOnly')},
          {value: 'disable_non_proxied_udp', name: loadTimeData.getString('disableNonProxiedUdp')}
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
  },

  /** @private {?BravePrivacyBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BravePrivacyBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    // Used for first time initialization of checked state.
    // Can't use `prefs` property of `settings-toggle-button` directly
    // because p3a enabled is a local state setting, but PrefControlMixin
    // checks for a pref being valid, so have to fake it, same as upstream.
    const setP3AEnabledPref = (enabled) => this.setP3AEnabledPref_(enabled);
    this.addWebUIListener('p3a-enabled-changed', setP3AEnabledPref);
    this.browserProxy_.getP3AEnabled().then(setP3AEnabledPref);

    const setStatsUsagePingEnabledPref = (enabled) => this.setStatsUsagePingEnabledPref_(enabled);
    this.addWebUIListener('stats-usage-ping-enabled-changed', setStatsUsagePingEnabledPref);
    this.browserProxy_.getStatsUsagePingEnabled().then(setStatsUsagePingEnabledPref);
  },

  setP3AEnabledPref_: function (enabled) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.p3aEnabledPref_ = pref;
  },

  onP3AEnabledChange_: function() {
    this.browserProxy_.setP3AEnabled(this.$.p3aEnabled.checked);
  },

  setStatsUsagePingEnabledPref_: function (enabled) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.statsUsagePingEnabledPref_ = pref;
  },

  onStatsUsagePingEnabledChange_: function() {
    this.browserProxy_.setStatsUsagePingEnabled(this.$.statsUsagePingEnabled.checked);
  },

  shouldShowRestart_: function(enabled) {
    return enabled != this.browserProxy_.wasPushMessagingEnabledAtStartup();
  },

  restartBrowser_: function(e) {
    e.stopPropagation();
    window.open("chrome://restart", "_self");
  },

});
