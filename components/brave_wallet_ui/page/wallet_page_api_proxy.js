// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy.js'
import { addSingletonGetter } from 'chrome://resources/js/cr.m.js'

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
      this.ethTxController.$.bindNewPipeAndPassReceiver());
  }
}

addSingletonGetter(WalletPageApiProxyImpl);
