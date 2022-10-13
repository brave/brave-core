/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {sendWithPromise} from 'chrome://resources/js/cr.m.js';

export interface BraveWalletBrowserProxy {
  setBraveWalletEnabled (value: boolean)
  getWeb3ProviderList () // TODO(petemill): Define the expected type
  getSolanaProviderList () // TODO(petemill): Define the expected type
  isNativeWalletEnabled () // TODO(petemill): Define the expected type
  getAutoLockMinutes () // TODO(petemill): Define the expected type
  getNetworksList (coin) // TODO(petemill): Define the expected type
  getPrepopulatedNetworksList () // TODO(petemill): Define the expected type
  removeChain (chainId, coin) // TODO(petemill): Define the expected type
  resetChain (chainId, coin) // TODO(petemill): Define the expected type
  addChain (value) // TODO(petemill): Define the expected type
  addHiddenNetwork (chainId, coin) // TODO(petemill): Define the expected type
  removeHiddenNetwork (chainId, coin) // TODO(petemill): Define the expected type
  setActiveNetwork (chainId, coin) // TODO(petemill): Define the expected type
  resetTransactionInfo ()
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

  static getInstance() {
    return instance || (instance = new BraveWalletBrowserProxyImpl())
  }
}

let instance: BraveWalletBrowserProxy|null = null
