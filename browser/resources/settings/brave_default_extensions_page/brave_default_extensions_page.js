/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

/**
 * 'settings-brave-default-extensions-page' is the settings page containing
 * brave's default extensions.
 */
Polymer({
  is: 'settings-brave-default-extensions-page',

  behaviors: [
    WebUIListenerBehavior,
  ],

  /**
   * Keep it the same as Provider in
   * brave/componentsdecentralized_dns/constants.h.
   */
  Provider: {
    UNSTOPPABLE_DOMAINS: 0,
    ENS: 1
  },

  properties: {
    showRestartToast_: Boolean,
    disableTorOption_: Boolean,
    decentralizedDnsEnabled_: Boolean,
    unstoppableDomainsResolveMethod_: Array,
    ensResolveMethod_: Array,
    torEnabledPref_: {
      type: Object,
      value() {
        // TODO(dbeam): this is basically only to appease PrefControlMixin.
        // Maybe add a no-validate attribute instead? This makes little sense.
        return {};
      },
    },
    widevineEnabledPref_: {
      type: Object,
      value() {
        // TODO(dbeam): this is basically only to appease PrefControlMixin.
        // Maybe add a no-validate attribute instead? This makes little sense.
        return {};
      },
    },
  },

  /** @private {?settings.BraveDefaultExtensionsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveDefaultExtensionsBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onWebTorrentEnabledChange_ = this.onWebTorrentEnabledChange_.bind(this)
    this.onHangoutsEnabledChange_ = this.onHangoutsEnabledChange_.bind(this)
    this.openExtensionsPage_ = this.openExtensionsPage_.bind(this)
    this.openKeyboardShortcutsPage_ = this.openKeyboardShortcutsPage_.bind(this)
    this.onWidevineEnabledChange_ = this.onWidevineEnabledChange_.bind(this)
    this.restartBrowser_ = this.restartBrowser_.bind(this)
    this.onTorEnabledChange_ = this.onTorEnabledChange_.bind(this)

    this.addWebUIListener('brave-needs-restart-changed', (needsRestart) => {
      this.showRestartToast_ = needsRestart
    })

    this.browserProxy_.getRestartNeeded().then(show => {
      this.showRestartToast_ = show;
    });
    this.browserProxy_.isTorManaged().then(managed => {
      this.disableTorOption_ = managed
    })
    this.browserProxy_.isDecentralizedDnsEnabled().then(enabled => {
      this.decentralizedDnsEnabled_ = enabled
    })
    this.browserProxy_.getDecentralizedDnsResolveMethodList(
      this.Provider.UNSTOPPABLE_DOMAINS).then(list => {
        this.unstoppableDomainsResolveMethod_ = list
    })
    this.browserProxy_.getDecentralizedDnsResolveMethodList(
      this.Provider.ENS).then(list => {
      this.ensResolveMethod_ = list
    })

    // PrefControlMixin checks for a pref being valid, so have to fake it,
    // same as upstream.
    const setTorEnabledPref = (enabled) => this.setTorEnabledPref_(enabled);
    this.addWebUIListener('tor-enabled-changed', setTorEnabledPref);
    this.browserProxy_.isTorEnabled().then(setTorEnabledPref);

    const setWidevineEnabledPref = (enabled) => this.setWidevineEnabledPref_(enabled);
    this.addWebUIListener('widevine-enabled-changed', setWidevineEnabledPref);
    this.browserProxy_.isWidevineEnabled().then(setWidevineEnabledPref);
  },

  onWebTorrentEnabledChange_: function() {
    this.browserProxy_.setWebTorrentEnabled(this.$.webTorrentEnabled.checked);
  },

  onHangoutsEnabledChange_: function() {
    this.browserProxy_.setHangoutsEnabled(this.$.hangoutsEnabled.checked);
  },

  restartBrowser_: function(e) {
    e.stopPropagation();
    window.open("chrome://restart", "_self");
  },

  setTorEnabledPref_: function (enabled) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.torEnabledPref_ = pref;
  },

  onTorEnabledChange_: function() {
    this.browserProxy_.setTorEnabled(this.$.torEnabled.checked);
  },

  setWidevineEnabledPref_: function (enabled) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
    this.widevineEnabledPref_ = pref;
  },

  onWidevineEnabledChange_: function() {
    this.browserProxy_.setWidevineEnabled(this.$.widevineEnabled.checked);
  },

  openExtensionsPage_: function() {
    window.open("chrome://extensions", "_self");
  },

  openKeyboardShortcutsPage_: function() {
    window.open("chrome://extensions/shortcuts", "_self");
  },

  openWebStoreUrl_: function() {
    window.open(loadTimeData.getString('getMoreExtensionsUrl'));
  },

  shouldShowRestartForGoogleLogin_: function(value) {
    return this.browserProxy_.wasSignInEnabledAtStartup() != value;
  },

  shouldShowRestartForMediaRouter_: function(value) {
    return this.browserProxy_.isMediaRouterEnabled() != value;
  }

});
})();
