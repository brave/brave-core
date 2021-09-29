/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {PrefsBehavior} from '../prefs/prefs_behavior.js';
 
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
    PrefsBehavior,
  ],

  properties: {
    isNativeWalletEnabled_: Boolean
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
      this.wallets_ = JSON.parse(list)
    });
    this.browserProxy_.isNativeWalletEnabled().then(val => {
      this.isNativeWalletEnabled_ = val;
    });
    this.onChangeAutoLockMinutes_ = this.onChangeAutoLockMinutes_.bind(this)
  },

  onBraveWalletEnabledChange_: function() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  },

  onChangeAutoLockMinutes_: function() {
    let value = Number(this.$.walletAutoLockMinutes.value)
    if (Number.isNaN(value) || value < 1 || value > 10080) {
      return
    }
    this.setPrefValue('brave.wallet.auto_lock_minutes', value)
  },
});
})();
