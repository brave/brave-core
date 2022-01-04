// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as WalletActions from '../common/actions/wallet_actions'
import { Store } from './async/types'
import { getBraveKeyring } from './api/hardware_keyrings'
import { BraveWallet } from '../constants/types'

export default class WalletApiProxy {
  walletHandler = new BraveWallet.WalletHandlerRemote()
  ethJsonRpcController = new BraveWallet.EthJsonRpcControllerRemote()
  swapService = new BraveWallet.SwapServiceRemote()
  assetRatioService = new BraveWallet.AssetRatioServiceRemote()

  keyringController = getBraveKeyring()
  ercTokenRegistry = new BraveWallet.ERCTokenRegistryRemote()
  ethTxController = new BraveWallet.EthTxControllerRemote()
  braveWalletService = new BraveWallet.BraveWalletServiceRemote()

  addEthJsonRpcControllerObserver (store: Store) {
    const ethJsonRpcControllerObserverReceiver = new BraveWallet.EthJsonRpcControllerObserverReceiver({
      chainChangedEvent: function (chainId) {
        store.dispatch(WalletActions.chainChangedEvent({ chainId }))
      },
      onAddEthereumChainRequestCompleted: function (chainId, error) {
        // TODO: Handle this event.
      },
      onIsEip1559Changed: function (chainId, isEip1559) {
        store.dispatch(WalletActions.isEip1559Changed({ chainId, isEip1559 }))
      }
    })
    this.ethJsonRpcController.addObserver(ethJsonRpcControllerObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addKeyringControllerObserver (store: Store) {
    const keyringControllerObserverReceiver = new BraveWallet.KeyringControllerObserverReceiver({
      keyringCreated: function () {
        store.dispatch(WalletActions.keyringCreated())
      },
      keyringRestored: function () {
        store.dispatch(WalletActions.keyringRestored())
      },
      keyringReset: function () {
        store.dispatch(WalletActions.keyringReset())
      },
      locked: function () {
        store.dispatch(WalletActions.locked())
      },
      unlocked: function () {
        store.dispatch(WalletActions.unlocked())
      },
      backedUp: function () {
        store.dispatch(WalletActions.backedUp())
      },
      accountsChanged: function () {
        store.dispatch(WalletActions.accountsChanged())
      },
      autoLockMinutesChanged: function () {
        store.dispatch(WalletActions.autoLockMinutesChanged())
      },
      selectedAccountChanged: function () {
        store.dispatch(WalletActions.selectedAccountChanged())
      }
    })
    this.keyringController.addObserver(keyringControllerObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addEthTxControllerObserverObserver (store: Store) {
    const ethTxControllerObserverReceiver = new BraveWallet.EthTxControllerObserverReceiver({
      onNewUnapprovedTx: function (txInfo) {
        store.dispatch(WalletActions.newUnapprovedTxAdded({ txInfo }))
      },
      onUnapprovedTxUpdated: function (txInfo) {
        store.dispatch(WalletActions.unapprovedTxUpdated({ txInfo }))
      },
      onTransactionStatusChanged: function (txInfo) {
        store.dispatch(WalletActions.transactionStatusChanged({ txInfo }))
      }
    })
    this.ethTxController.addObserver(ethTxControllerObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletServiceObserver (store: Store) {
    const braveWalletServiceObserverReceiver = new BraveWallet.BraveWalletServiceObserverReceiver({
      onActiveOriginChanged: function (origin) {
        store.dispatch(WalletActions.activeOriginChanged({ origin }))
      },
      onDefaultWalletChanged: function (defaultWallet) {
        store.dispatch(WalletActions.defaultWalletChanged({ defaultWallet }))
      },
      onDefaultBaseCurrencyChanged: function (currency) {
        store.dispatch(WalletActions.defaultBaseCurrencyChanged({ currency }))
      },
      onDefaultBaseCryptocurrencyChanged: function (cryptocurrency) {
        store.dispatch(WalletActions.defaultBaseCryptocurrencyChanged({ cryptocurrency }))
      },
      onNetworkListChanged: function () {
        store.dispatch(WalletActions.getAllNetworks())
      }
    })
    this.braveWalletService.addObserver(braveWalletServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }
}
