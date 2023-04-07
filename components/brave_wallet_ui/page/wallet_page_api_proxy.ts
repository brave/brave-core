// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet } from '../constants/types'

let walletPageApiProxyInstance: WalletPageApiProxy

class WalletPageApiProxy extends WalletApiProxy {
  callbackRouter = new BraveWallet.PageCallbackRouter()
  pageHandler = new BraveWallet.PageHandlerRemote()
  constructor () {
    super()

    const factory = BraveWallet?.PageHandlerFactory?.getRemote?.()
    factory?.createPageHandler?.(
      this.callbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver(),
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
      this.braveWalletP3A.$.bindNewPipeAndPassReceiver(),
      this.braveWalletPinService.$.bindNewPipeAndPassReceiver(),
      this.braveWalletAutoPinService.$.bindNewPipeAndPassReceiver(),
      this.braveWalletIpfsService.$.bindNewPipeAndPassReceiver())
  }
}

export default function getWalletPageApiProxy () {
  if (!walletPageApiProxyInstance) {
    walletPageApiProxyInstance = new WalletPageApiProxy()
  }
  return walletPageApiProxyInstance
}
