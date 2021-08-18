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
    torEnabled_: Boolean,
    widevineEnabled_: Boolean,
    disableTorOption_: Boolean,
    decentralizedDnsEnabled_: Boolean,
    unstoppableDomainsResolveMethod_: Array,
    ensResolveMethod_: Array,
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
    this.addWebUIListener('tor-enabled-changed', (enabled) => {
      this.torEnabled_ = enabled
    })
    this.addWebUIListener('widevine-enabled-changed', (enabled) => {
      this.widevineEnabled_ = enabled
    })

    this.browserProxy_.getRestartNeeded().then(show => {
      this.showRestartToast_ = show;
    });
    this.browserProxy_.isTorEnabled().then(enabled => {
      this.torEnabled_ = enabled
    })
    this.browserProxy_.isTorManaged().then(managed => {
      this.disableTorOption_ = managed
    })
    this.browserProxy_.isWidevineEnabled().then(enabled => {
      this.widevineEnabled_ = enabled
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

  onTorEnabledChange_: function() {
    this.browserProxy_.setTorEnabled(this.$.torEnabled.checked);
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
  }

});
})();
