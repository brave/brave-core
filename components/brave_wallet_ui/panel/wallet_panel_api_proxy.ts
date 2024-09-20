// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet } from '../constants/types'

export class WalletPanelApiProxy extends WalletApiProxy {
  panelHandler = new BraveWallet.PanelHandlerRemote()

  constructor() {
    super()

    const factory = BraveWallet.PanelHandlerFactory.getRemote()
    factory.createPanelHandler(
      this.panelHandler.$.bindNewPipeAndPassReceiver(),
      this.walletHandler.$.bindNewPipeAndPassReceiver(),
      this.jsonRpcService.$.bindNewPipeAndPassReceiver(),
      this.bitcoinWalletService.$.bindNewPipeAndPassReceiver(),
      this.zcashWalletService.$.bindNewPipeAndPassReceiver(),
      this.swapService.$.bindNewPipeAndPassReceiver(),
      this.simulationService.$.bindNewPipeAndPassReceiver(),
      this.assetRatioService.$.bindNewPipeAndPassReceiver(),
      this.keyringService.$.bindNewPipeAndPassReceiver(),
      this.blockchainRegistry.$.bindNewPipeAndPassReceiver(),
      this.txService.$.bindNewPipeAndPassReceiver(),
      this.ethTxManagerProxy.$.bindNewPipeAndPassReceiver(),
      this.solanaTxManagerProxy.$.bindNewPipeAndPassReceiver(),
      this.filTxManagerProxy.$.bindNewPipeAndPassReceiver(),
      this.btcTxManagerProxy.$.bindNewPipeAndPassReceiver(),
      this.braveWalletService.$.bindNewPipeAndPassReceiver(),
      this.braveWalletP3A.$.bindNewPipeAndPassReceiver(),
      this.braveWalletIpfsService.$.bindNewPipeAndPassReceiver(),
      this.meldIntegrationService.$.bindNewPipeAndPassReceiver()
    )
  }
}

let walletPanelApiProxyInstance: WalletPanelApiProxy

export default function getWalletPanelApiProxy() {
  if (!walletPanelApiProxyInstance) {
    walletPanelApiProxyInstance = new WalletPanelApiProxy()
  }
  return walletPanelApiProxyInstance
}
