// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { addSingletonGetter } from 'chrome://resources/js/cr.m.js'
import WalletApiProxy from '../common/wallet_api_proxy.js'

// TODO(petemill): Convert this module to Typescript, and use
// es-module versions of mojom bindings, e.g.
// import * as BraveWallet from '../common/wallet_api_proxy'
// ...
// const factory = BraveWallet.WalletHandler.getRemote()
// ...

/** @interface */
class WalletPageApiProxy {}

/** @implements {WalletPageApiProxy} */
export default class WalletPageApiProxyImpl extends WalletApiProxy {
  constructor() {
    super()
    /** @type {!braveWallet.mojom.PageCallbackRouter} */
    this.callbackRouter = new braveWallet.mojom.PageCallbackRouter();
    /** @type {!braveWallet.mojom.PageHandlerRemote} */
    this.pageHandler = new braveWallet.mojom.PageHandlerRemote();

    const factory = braveWallet.mojom.PageHandlerFactory.getRemote();
    factory.createPageHandler(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver(),
      this.walletHandler.$.bindNewPipeAndPassReceiver(),
      this.ethJsonRpcController.$.bindNewPipeAndPassReceiver(),
      this.swapController.$.bindNewPipeAndPassReceiver(),
      this.assetRatioController.$.bindNewPipeAndPassReceiver(),
      this.keyringController.$.bindNewPipeAndPassReceiver(),
      this.ercTokenRegistry.$.bindNewPipeAndPassReceiver(),
      this.ethTxController.$.bindNewPipeAndPassReceiver(),
      this.braveWalletService.$.bindNewPipeAndPassReceiver(),
      this.trezorBridgeController.$.bindNewPipeAndPassReceiver());
  }
}

// TODO(petemill): Use module-scoped variable and custom `getInstance` static
// function, since `addSingletonGetter` doesn't work well with Typescript.
addSingletonGetter(WalletPageApiProxyImpl);
