// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

export class WalletApiProxy {
  walletHandler = new BraveWallet.WalletHandlerRemote()
  jsonRpcService = new BraveWallet.JsonRpcServiceRemote()
  bitcoinWalletService = new BraveWallet.BitcoinWalletServiceRemote()
  swapService = new BraveWallet.SwapServiceRemote()
  simulationService = new BraveWallet.SimulationServiceRemote()
  assetRatioService = new BraveWallet.AssetRatioServiceRemote()
  keyringService = new BraveWallet.KeyringServiceRemote()
  blockchainRegistry = new BraveWallet.BlockchainRegistryRemote()
  txService = new BraveWallet.TxServiceRemote()
  ethTxManagerProxy = new BraveWallet.EthTxManagerProxyRemote()
  solanaTxManagerProxy = new BraveWallet.SolanaTxManagerProxyRemote()
  filTxManagerProxy = new BraveWallet.FilTxManagerProxyRemote()
  braveWalletService = new BraveWallet.BraveWalletServiceRemote()
  braveWalletP3A = new BraveWallet.BraveWalletP3ARemote()
  braveWalletPinService = new BraveWallet.WalletPinServiceRemote()
  braveWalletAutoPinService = new BraveWallet.WalletAutoPinServiceRemote()
  braveWalletIpfsService = new BraveWallet.IpfsServiceRemote()

  addJsonRpcServiceObserver (observer: BraveWallet.JsonRpcServiceObserverReceiver) {
    this.jsonRpcService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addKeyringServiceObserver (observer: BraveWallet.KeyringServiceObserverReceiver) {
    this.keyringService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addTxServiceObserver (observer: BraveWallet.TxServiceObserverReceiver) {
    this.txService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletServiceObserver (observer: BraveWallet.BraveWalletServiceObserverReceiver) {
    this.braveWalletService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletServiceTokenObserver (observer: BraveWallet.BraveWalletServiceTokenObserverReceiver) {
    this.braveWalletService.addTokenObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletPinServiceObserver (observer: BraveWallet.BraveWalletPinServiceObserverReceiver) {
    this.braveWalletPinService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletAutoPinServiceObserver (observer: BraveWallet.WalletAutoPinServiceObserverReceiver) {
    this.braveWalletAutoPinService.addObserver(observer.$.bindNewPipeAndPassRemote())
  }
}

export default WalletApiProxy
