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
class WalletPageApiProxy {}

/** @implements {WalletPageApiProxy} */
export default class WalletPageApiProxyImpl {
  constructor() {
    /** @type {!braveWallet.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveWallet.mojom.PageCallbackRouter();
    /** @type {!braveWallet.mojom.PageHandlerRemote} */
    this.pageHandler = new braveWallet.mojom.PageHandlerRemote();
    /** @type {!braveWallet.mojom.WalletHandlerRemote} */
    this.walletHandler = new braveWallet.mojom.WalletHandlerRemote();
    /** @type {!braveWallet.mojom.EthJsonRpcControllerRemote} */
    this.ethJsonRpcController = new braveWallet.mojom.EthJsonRpcControllerRemote();
    /** @type {!braveWallet.mojom.SwapControllerRemote} */
    this.swapController = new braveWallet.mojom.SwapControllerRemote();
    /** @type {!braveWallet.mojom.AssetRatioControllerRemote} */
    this.assetRatioController = new braveWallet.mojom.AssetRatioControllerRemote();
    /** @type {!braveWallet.mojom.KeyringControllerRemote} */
    this.keyringController = new braveWallet.mojom.KeyringControllerRemote();


    const factory = braveWallet.mojom.PageHandlerFactory.getRemote();
    factory.createPageHandler(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver(),
      this.walletHandler.$.bindNewPipeAndPassReceiver(),
      this.ethJsonRpcController.$.bindNewPipeAndPassReceiver(),
      this.swapController.$.bindNewPipeAndPassReceiver(),
      this.assetRatioController.$.bindNewPipeAndPassReceiver(),
      this.keyringController.$.bindNewPipeAndPassReceiver());
  }
}

addSingletonGetter(WalletPageApiProxyImpl);
