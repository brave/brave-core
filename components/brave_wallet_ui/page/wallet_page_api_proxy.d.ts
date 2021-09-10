// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { WalletAPIHandler } from '../constants/types'

export class PageHandler {
  showApprovePanelUI: () => Promise<void>
}

export default class APIProxy {
  static getInstance: () => APIProxy
  walletHandler: WalletAPIHandler
  pageHandler: PageHandler
  ethJsonRpcController: EthJsonRpcController
  swapController: SwapController
  assetRatioController: AssetRatioController
  ercTokenRegistry: ERCTokenRegistry
  keyringController: KeyringController
  ethTxController: EthTxController
  addEthJsonRpcControllerObserver: (store: any) => void
  addKeyringControllerObserver: (store: any) => void
  addEthTxControllerObserverObserver: (store: any) => void
  makeTxData: (nonce: string, gasPrice: string, gasLimit: string, to: string, value: string) => any
}
