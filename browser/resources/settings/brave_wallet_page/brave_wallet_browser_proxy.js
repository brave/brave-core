/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter, sendWithPromise} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveWalletBrowserProxy {
  /**
   * @param {boolean} value name.
   */
   setBraveWalletEnabled (value) {}
   getWeb3ProviderList () {}
   isNativeWalletEnabled() {}
   getAutoLockMinutes() {}
   getCustomNetworksList () {}
   removeEthereumChain (chainId) {}
   addEthereumChain (value) {}
   setActiveNetwork (chainId) {}
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
  resetTransactionInfo() {
    chrome.send('resetTransactionInfo', [])
  }
  /** @override */
  setBraveWalletEnabled (value) {
    chrome.send('setBraveWalletEnabled', [value])
  }

  getCustomNetworksList () {
    return sendWithPromise('getCustomNetworksList')
  }

  setActiveNetwork (chainId) {
    return sendWithPromise('setActiveNetwork', chainId)
  }

  removeEthereumChain (chainId) {
    return sendWithPromise('removeEthereumChain', chainId)
  }

  addEthereumChain (payload) {
    return sendWithPromise('addEthereumChain', payload)
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
  getAutoLockMinutes() {
    return sendWithPromise('getAutoLockMinutes')
  }
}

addSingletonGetter(BraveWalletBrowserProxyImpl)
