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
  is: 'settings-brave-wallet-page',

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
  },

  /** @private {?settings.BraveWalletBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveWalletBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onBraveWalletEnabledChange_ = this.onBraveWalletEnabledChange_.bind(this)
    this.browserProxy_.getWeb3ProviderList().then(list => {
      this.braveWeb3Providers_ = JSON.parse(list)
    });
  },

  onBraveWalletEnabledChange_: function() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  },
});
})();
