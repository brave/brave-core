// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/mojo/mojo/public/js/mojo_bindings_lite.js'
import 'chrome://resources/mojo/url/mojom/url.mojom-lite.js'
import 'chrome://resources/mojo/mojo/public/mojom/base/time.mojom-lite.js';
import 'chrome://resources/mojo/brave/components/brave_wallet/common/brave_wallet.mojom-lite.js'

import { addSingletonGetter } from 'chrome://resources/js/cr.m.js'

/** @interface */
class WalletPageApiProxy {
  /**
   * @param {string} password
   * @param {string} mnemonic
   */
  createWallet (password) { }

  restoreWallet (mnemonic, password) { }

  getRecoveryWords (password) { }

  notifyWalletBackupComplete () { }

  /** @return {!braveWallet.mojom.PageCallbackRouter} */
  getCallbackRouter () { }

  getWalletHandler () { }
}

/** @implements {WalletPageApiProxy} */
export default class WalletPageApiProxyImpl {
  constructor() {
    /** @type {!braveWallet.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveWallet.mojom.PageCallbackRouter();

    /** @type {!braveWallet.mojom.PageHandlerRemote} */
    this.page_handler = new braveWallet.mojom.PageHandlerRemote();
    /** @type {!braveWallet.mojom.WalletHandlerRemote} */
    this.wallet_handler = new braveWallet.mojom.WalletHandlerRemote();

    const factory = braveWallet.mojom.PageHandlerFactory.getRemote();
    factory.createPageHandler(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.page_handler.$.bindNewPipeAndPassReceiver(),
      this.wallet_handler.$.bindNewPipeAndPassReceiver());
  }

  /** @override */
  createWallet (password) {
    return this.page_handler.createWallet(password);
  }

  /** @override */
  restoreWallet (mnemonic, password) {
    return this.page_handler.restoreWallet(mnemonic, password);
  }

  /** @override */
  getRecoveryWords (password) {
    return this.page_handler.getRecoveryWords();
  }

  /** @override */
  notifyWalletBackupComplete () {
    return this.wallet_handler.notifyWalletBackupComplete();
  }

  /** @override */
  getCallbackRouter () {
    return this.callbackRouter;
  }

  /** @override */
  getWalletHandler () {
    return this.wallet_handler;
  }
}

addSingletonGetter(WalletPageApiProxyImpl);
