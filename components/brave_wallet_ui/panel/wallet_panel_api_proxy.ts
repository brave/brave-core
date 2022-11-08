// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet } from '../constants/types'

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
        this.jsonRpcService.$.bindNewPipeAndPassReceiver(),
        this.swapService.$.bindNewPipeAndPassReceiver(),
        this.assetRatioService.$.bindNewPipeAndPassReceiver(),
        this.keyringService.$.bindNewPipeAndPassReceiver(),
        this.blockchainRegistry.$.bindNewPipeAndPassReceiver(),
        this.txService.$.bindNewPipeAndPassReceiver(),
        this.ethTxManagerProxy.$.bindNewPipeAndPassReceiver(),
        this.solanaTxManagerProxy.$.bindNewPipeAndPassReceiver(),
        this.filTxManagerProxy.$.bindNewPipeAndPassReceiver(),
        this.braveWalletService.$.bindNewPipeAndPassReceiver(),
        this.braveWalletP3A.$.bindNewPipeAndPassReceiver())
  }
}

let walletPanelApiProxyInstance: WalletPanelApiProxy

export default function getWalletPanelApiProxy () {
  if (!walletPanelApiProxyInstance) {
    walletPanelApiProxyInstance = new WalletPanelApiProxy()
  }
  return walletPanelApiProxyInstance
}
