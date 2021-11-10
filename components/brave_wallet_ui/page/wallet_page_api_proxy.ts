// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import WalletApiProxy, * as BraveWallet from '../common/wallet_api_proxy'

let walletPageApiProxyInstance: WalletPageApiProxy

class WalletPageApiProxy extends WalletApiProxy {
  callbackRouter = new BraveWallet.PageCallbackRouter()
  pageHandler = new BraveWallet.PageHandlerRemote()
  constructor () {
    super()

    const factory = BraveWallet.PageHandlerFactory.getRemote()
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
      this.braveWalletService.$.bindNewPipeAndPassReceiver())
  }
}

export default function getWalletPageApiProxy () {
  if (!walletPageApiProxyInstance) {
    walletPageApiProxyInstance = new WalletPageApiProxy()
  }
  return walletPageApiProxyInstance
}
