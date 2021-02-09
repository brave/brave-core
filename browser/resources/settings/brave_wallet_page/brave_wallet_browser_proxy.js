/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {addSingletonGetter} from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveWalletBrowserProxy {
  /**
   * @param {boolean} value name.
   */
   setBraveWalletEnabled (value) {}
   getWeb3ProviderList () {}
}

/**
 * @implements {settings.BraveWalletBrowserProxy}
 */
export class BraveWalletBrowserProxyImpl {
  /** @override */
  setBraveWalletEnabled (value) {
    chrome.send('setBraveWalletEnabled', [value])
  }

  /** @override */
  getWeb3ProviderList () {
    return new Promise(resolve => chrome.braveWallet.getWeb3ProviderList(resolve))
  }
}

addSingletonGetter(BraveWalletBrowserProxyImpl)
