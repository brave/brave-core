/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

//import {PrefsBehavior} from '../prefs/prefs_behavior.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
 
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
    //PrefsBehavior,
    I18nBehavior
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
    this.onInputAutoLockMinutes_ = this.onInputAutoLockMinutes_.bind(this)
    this.onResetWallet_ = this.onResetWallet_.bind(this)
    this.browserProxy_.getWeb3ProviderList().then(list => {
      this.wallets_ = JSON.parse(list)
    });
    this.browserProxy_.isNativeWalletEnabled().then(val => {
      this.isNativeWalletEnabled_ = val;
    });
    this.browserProxy_.getAutoLockMinutes().then(val => {
      this.$.walletAutoLockMinutes.value = val
    })
  },

  onBraveWalletEnabledChange_: function() {
    this.browserProxy_.setBraveWalletEnabled(this.$.braveWalletEnabled.checked);
  },

  onInputAutoLockMinutes_: function() {
    let value = Number(this.$.walletAutoLockMinutes.value)
    if (Number.isNaN(value) || value < 1 || value > 10080) {
      return
    }
    this.setPrefValue('brave.wallet.auto_lock_minutes', value)
  },

  onResetWallet_: function() {
    var message = this.i18n('walletResetConfirmation')
    if (window.prompt(message) !== this.i18n('walletResetConfirmationPhrase'))
      return
    this.browserProxy_.resetWallet();
  }
});
})();
