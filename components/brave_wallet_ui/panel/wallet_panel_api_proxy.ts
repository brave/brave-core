// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import WalletApiProxy, * as BraveWallet from '../common/wallet_api_proxy'

class WalletPanelApiProxy extends WalletApiProxy {
  callbackRouter = new BraveWallet.PageCallbackRouter()
  panelHandler = new BraveWallet.PanelHandlerRemote()

  constructor () {
    super()

    const factory = BraveWallet.PanelHandlerFactory.getRemote()
    factory.createPanelHandler(
        this.callbackRouter.$.bindNewPipeAndPassRemote(),
        this.panelHandler.$.bindNewPipeAndPassReceiver(),
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

let walletPanelApiProxyInstance: WalletPanelApiProxy

export default function getWalletPanelApiProxy () {
  if (!walletPanelApiProxyInstance) {
    walletPanelApiProxyInstance = new WalletPanelApiProxy()
  }
  return walletPanelApiProxyInstance
}
