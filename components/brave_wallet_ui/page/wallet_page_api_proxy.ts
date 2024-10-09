// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet } from '../constants/types'

let walletPageApiProxyInstance: WalletPageApiProxy

export class WalletPageApiProxy extends WalletApiProxy {
  pageHandler = new BraveWallet.PageHandlerRemote()
  constructor() {
    super()

    const factory = BraveWallet?.PageHandlerFactory?.getRemote?.()
    factory?.createPageHandler?.(
      this.pageHandler.$.bindNewPipeAndPassReceiver(),
      this.walletHandler.$.bindNewPipeAndPassReceiver(),
      this.jsonRpcService.$.bindNewPipeAndPassReceiver(),
      this.bitcoinWalletService.$.bindNewPipeAndPassReceiver(),
      this.zcashWalletService.$.bindNewPipeAndPassReceiver(),
      this.swapService.$.bindNewPipeAndPassReceiver(),
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

export default function getWalletPageApiProxy() {
  if (!walletPageApiProxyInstance) {
    walletPageApiProxyInstance = new WalletPageApiProxy()
  }
  return walletPageApiProxyInstance
}
