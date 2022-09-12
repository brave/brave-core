/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveWalletBrowserProxy {
  setBraveWalletEnabled (value) {}
  getWeb3ProviderList () {}
  getSolanaProviderList () {}
  isNativeWalletEnabled () {}
  getAutoLockMinutes () {}
  getNetworksList (coin) {}
  getPrepopulatedNetworksList () {}
  removeChain (chainId, coin) {}
  resetChain (chainId, coin) {}
  addChain (value) {}
  addHiddenNetwork (chainId, coin) {}
  removeHiddenNetwork (chainId, coin) {}
  setActiveNetwork (chainId, coin) {}
  resetTransactionInfo () {}
}

/**
 * @implements {settings.BraveWalletBrowserProxy}
 */
export class BraveWalletBrowserProxyImpl {
  /** @override */
  resetWallet () {
    chrome.send('resetWallet', [])
  }
  /** @override */
  resetTransactionInfo () {
    chrome.send('resetTransactionInfo', [])
  }
  /** @override */
  setBraveWalletEnabled (value) {
    chrome.send('setBraveWalletEnabled', [value])
  }

  /** @override */
  getNetworksList (coin) {
    return sendWithPromise('getNetworksList', coin)
  }

  /** @override */
  getPrepopulatedNetworksList () {
    return sendWithPromise('getPrepopulatedNetworksList')
  }

  /** @override */
  setActiveNetwork (chainId, coin) {
    return sendWithPromise('setActiveNetwork', chainId, coin)
  }

  /** @override */
  removeChain (chainId, coin) {
    return sendWithPromise('removeChain', chainId, coin)
  }

  /** @override */
  resetChain (chainId, coin) {
    return sendWithPromise('resetChain', chainId, coin)
  }

  /** @override */
  addChain (payload) {
    return sendWithPromise('addChain', payload)
  }

  /** @override */
  addHiddenNetwork (chainId, coin) {
    return sendWithPromise('addHiddenNetwork', chainId, coin)
  }

  /** @override */
  removeHiddenNetwork (chainId, coin) {
    return sendWithPromise('removeHiddenNetwork', chainId, coin)
  }

  /** @override */
  getWeb3ProviderList () {
    return new Promise(resolve => chrome.braveWallet.getWeb3ProviderList(resolve))
  }

  /** @override */
  isNativeWalletEnabled () {
    return new Promise(resolve => chrome.braveWallet.isNativeWalletEnabled(resolve))
  }

  /** @override */
  getAutoLockMinutes () {
    return sendWithPromise('getAutoLockMinutes')
  }

  /** @override */
  getSolanaProviderOptions() {
    return sendWithPromise('getSolanaProviderOptions')
  }
}

addSingletonGetter(BraveWalletBrowserProxyImpl)
