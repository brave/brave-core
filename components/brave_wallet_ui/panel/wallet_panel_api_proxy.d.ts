// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  APIProxyControllers,
  WalletAPIHandler
} from '../constants/types'

export default class APIProxy implements APIProxyControllers {
  static getInstance: () => APIProxy
  showUI: () => {}
  closeUI: () => {}
  connectToSite: (accounts: string[], origin: string, tabId: number) => {}
  cancelConnectToSite: (origin: string, tabId: number) => {}
  closePanelOnDeactivate: (close: Boolean) => {}
  walletHandler: WalletAPIHandler
  ethJsonRpcController: EthJsonRpcController
  swapController: SwapController
  assetRatioController: AssetRatioController
  ercTokenRegistry: ERCTokenRegistry
  keyringController: KeyringController
  ethTxController: EthTxController
  braveWalletService: BraveWalletService
  addEthJsonRpcControllerObserver: (store: any) => void
  addKeyringControllerObserver: (store: any) => void
  addEthTxControllerObserverObserver: (store: any) => void
  addBraveWalletServiceObserver: (store: any) => void
  getKeyringsByType: (type: string) => any
  makeTxData: (
    nonce: string,
    gasPrice: string,
    gasLimit: string,
    to: string,
    value: string,
    data: number[]
  ) => any
  makeEIP1559TxData: (
    chainId: string,
    nonce: string,
    maxPriorityFeePerGas: string,
    maxFeePerGas: string,
    gasLimit: string,
    to: string,
    value: string,
    data: number[]
  ) => any
}
