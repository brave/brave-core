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
   isNativeWalletEnabled () {}
   getAutoLockMinutes () {}
   getNetworksList () {}
   removeEthereumChain (chainId) {}
   resetEthereumChain (chainId) {}
   addEthereumChain (value) {}
   addHiddenNetwork (payload) {}
   removeHiddenNetwork (payload) {}
   searchNetworks (chain_id_filter, chain_name_filter) {}
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
  resetTransactionInfo () {
    chrome.send('resetTransactionInfo', [])
  }
  /** @override */
  setBraveWalletEnabled (value) {
    chrome.send('setBraveWalletEnabled', [value])
  }

  /** @override */
  getNetworksList() {
    return sendWithPromise('getNetworksList')
  }

  /** @override */
  setActiveNetwork (chainId) {
    return sendWithPromise('setActiveNetwork', chainId)
  }

  /** @override */
  removeEthereumChain (chainId) {
    return sendWithPromise('removeEthereumChain', chainId)
  }

  /** @override */
  resetEthereumChain (chainId) {
    return sendWithPromise('resetEthereumChain', chainId)
  }

  /** @override */
  addEthereumChain (payload) {
    return sendWithPromise('addEthereumChain', payload)
  }

  /** @override */
  addHiddenNetwork (payload) {
    return sendWithPromise('addHiddenNetwork', payload)
  }

  /** @override */
  removeHiddenNetwork (payload) {
    return sendWithPromise('removeHiddenNetwork', payload)
  }

  /** @override */
  searchNetworks (chain_id_filter, chain_name_filter) {
    return sendWithPromise('searchNetworks', chain_id_filter, chain_name_filter)
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
}

addSingletonGetter(BraveWalletBrowserProxyImpl)
