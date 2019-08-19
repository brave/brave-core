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

  /** @private {?settings.BraveDefaultExtensionsBrowserProxy} */
  browserProxy_: null,
  showRestartToast: false,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveDefaultExtensionsBrowserProxyImpl.getInstance();
    this.browserProxy_.getRestartNeeded().then(show => {
      this.showRestartToast = show;
    });
    this.addWebUIListener('brave-needs-restart-changed', (needsRestart) => {
      this.showRestartToast = needsRestart
    })
  },

  /** @override */
  ready: function() {
    this.onWebTorrentEnabledChange_ = this.onWebTorrentEnabledChange_.bind(this)
    this.onBraveWalletEnabledChange_ = this.onBraveWalletEnabledChange_.bind(this)
    this.onHangoutsEnabledChange_ = this.onHangoutsEnabledChange_.bind(this)
    this.onIPFSCompanionEnabledChange_ = this.onIPFSCompanionEnabledChange_.bind(this)
    this.openExtensionsPage_ = this.openExtensionsPage_.bind(this)
    this.restartBrowser_ = this.restartBrowser_.bind(this)
  },

  onWebTorrentEnabledChange_: function() {
    this.browserProxy_.setWebTorrentEnabled(this.$.webTorrentEnabled.checked);
  },

  onBraveWalletEnabledChange_: function() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  },

  onHangoutsEnabledChange_: function() {
    this.browserProxy_.setHangoutsEnabled(this.$.hangoutsEnabled.checked);
  },

  onIPFSCompanionEnabledChange_: function() {
    this.browserProxy_.setIPFSCompanionEnabled(this.$.ipfsCompanionEnabled.checked);
  },

  restartBrowser_: function() {
    window.open("chrome://restart", "_self");
  },

  onMediaRouterEnabledChange_: function() {
    this.browserProxy_.setMediaRouterEnabled(this.$.mediaRouterEnabled.checked);
  },

  openExtensionsPage_: function() {
    window.open("chrome://extensions", "_self");
  },
});
})();
