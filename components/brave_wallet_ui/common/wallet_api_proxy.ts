// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as WalletActions from '../common/actions/wallet_actions'
import { Store } from './async/types'
import { getBraveKeyring } from './api/hardware_keyrings'
import { BraveWallet } from '../constants/types'
import { objectEquals } from '../utils/object-utils'
import { makeSerializableTransaction } from '../utils/model-serialization-utils'
import { WalletPageActions } from '../page/actions'
import { walletApi } from './slices/api.slice'
import { getCoinFromTxDataUnion } from '../utils/network-utils'

export class WalletApiProxy {
  walletHandler = new BraveWallet.WalletHandlerRemote()
  jsonRpcService = new BraveWallet.JsonRpcServiceRemote()
  bitcoinWalletService = new BraveWallet.BitcoinWalletServiceRemote()
  swapService = new BraveWallet.SwapServiceRemote()
  simulationService = new BraveWallet.SimulationServiceRemote()
  assetRatioService = new BraveWallet.AssetRatioServiceRemote()
  keyringService = getBraveKeyring()
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

  addJsonRpcServiceObserver (store: Store) {
    const jsonRpcServiceObserverReceiver = new BraveWallet.JsonRpcServiceObserverReceiver({
      chainChangedEvent: function (chainId, coin, origin) {
        store.dispatch(walletApi.endpoints.invalidateSelectedChain.initiate())
      },
      onAddEthereumChainRequestCompleted: function (chainId, error) {
        // TODO: Handle this event.
      },
      onIsEip1559Changed: function (chainId, isEip1559) {
        store.dispatch(
          walletApi.endpoints.isEip1559Changed.initiate({ chainId, isEip1559 })
        )
      }
    })
    this.jsonRpcService.addObserver(jsonRpcServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addKeyringServiceObserver (store: Store) {
    const keyringServiceObserverReceiver = new BraveWallet.KeyringServiceObserverReceiver({
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
        store.dispatch(walletApi.endpoints.invalidateAccountInfos.initiate())
        store.dispatch(WalletActions.accountsChanged())
      },
      accountsAdded: function () {
        // TODO: Handle this event.
      },
      autoLockMinutesChanged: function () {
        store.dispatch(WalletActions.autoLockMinutesChanged())
      },
      selectedWalletAccountChanged: function (account: BraveWallet.AccountInfo) {
        store.dispatch(walletApi.endpoints.invalidateSelectedAccount.initiate())
      },
      selectedDappAccountChanged: function (coin: BraveWallet.CoinType, account: BraveWallet.AccountInfo | null) {
        // TODO: Handle this event.
      }
    })
    this.keyringService.addObserver(keyringServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addTxServiceObserver (store: Store) {
    const txServiceManagerObserverReceiver = new BraveWallet.TxServiceObserverReceiver({
      onNewUnapprovedTx: function (txInfo) {
        store.dispatch(
          walletApi.endpoints.newUnapprovedTxAdded.initiate(
            makeSerializableTransaction(txInfo)
          )
        )
      },
      onUnapprovedTxUpdated: function (txInfo) {
        store.dispatch(
          walletApi.endpoints.unapprovedTxUpdated.initiate(
            makeSerializableTransaction(txInfo)
          )
        )
      },
      onTransactionStatusChanged: (txInfo) => {
        store.dispatch(
          walletApi.endpoints.transactionStatusChanged.initiate({
            chainId: txInfo.chainId,
            coinType: getCoinFromTxDataUnion(txInfo.txDataUnion),
            fromAddress: txInfo.fromAddress,
            id: txInfo.id,
            txStatus: txInfo.txStatus
          })
        )

        // close then panel UI if there are no more pending transactions
        if (
          [
            BraveWallet.TransactionStatus.Submitted,
            BraveWallet.TransactionStatus.Signed,
            BraveWallet.TransactionStatus.Rejected,
            BraveWallet.TransactionStatus.Approved
          ].includes(txInfo.txStatus)
        ) {
          const state = store.getState()
          if (
            state.panel && // run only in panel
            state.ui.selectedPendingTransactionId === undefined &&
            (state.panel?.selectedPanel === 'approveTransaction' ||
              txInfo.txStatus === BraveWallet.TransactionStatus.Rejected)
          ) {
            store.dispatch(walletApi.endpoints.closePanelUI.initiate())
          }
        }
      },
      onTxServiceReset: function () {
      }
    })
    this.txService.addObserver(txServiceManagerObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletServiceObserver (store: Store) {
    const braveWalletServiceObserverReceiver = new BraveWallet.BraveWalletServiceObserverReceiver({
      onActiveOriginChanged: function (originInfo) {
        const state = store.getState().wallet

        // check that the origin has changed from the stored values
        // in any way before dispatching the update action
        if (objectEquals(state.activeOrigin, originInfo)) {
          return
        }

        store.dispatch(WalletActions.activeOriginChanged(originInfo))
      },
      onDefaultEthereumWalletChanged: function (defaultWallet) {
        store.dispatch(WalletActions.defaultEthereumWalletChanged({ defaultWallet }))
      },
      onDefaultSolanaWalletChanged: function (defaultWallet) {
        store.dispatch(WalletActions.defaultSolanaWalletChanged({ defaultWallet }))
      },
      onDefaultBaseCurrencyChanged: function (currency) {
        store.dispatch(WalletActions.defaultBaseCurrencyChanged({ currency }))
      },
      onDefaultBaseCryptocurrencyChanged: function (cryptocurrency) {
        store.dispatch(WalletActions.defaultBaseCryptocurrencyChanged({ cryptocurrency }))
      },
      onNetworkListChanged: function () {
        // FIXME(onyb): Due to a bug, the OnNetworkListChanged event is fired
        // merely upon switching to a custom network.
        //
        // Skipping balances refresh for now, until the bug is fixed.
        store.dispatch(WalletActions.refreshNetworksAndTokens({
          skipBalancesRefresh: true
        }))
      },
      onDiscoverAssetsStarted: function () { },
      onDiscoverAssetsCompleted: function (discoveredAssets) {
        store.dispatch(WalletActions.setAssetAutoDiscoveryCompleted(discoveredAssets))
      },
      onResetWallet: function () {
      }
    })
    this.braveWalletService.addObserver(braveWalletServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletPinServiceObserver (store: Store) {
    const braveWalletServiceObserverReceiver = new BraveWallet.BraveWalletPinServiceObserverReceiver({
      onTokenStatusChanged: function (service, token, status) {
        store.dispatch(WalletPageActions.updateNftPinningStatus({
          token,
          status
        }))
      },
      onLocalNodeStatusChanged: function (status) {
        store.dispatch(WalletPageActions.updateLocalIpfsNodeStatus(status))
      }
    })
    this.braveWalletPinService.addObserver(braveWalletServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }

  addBraveWalletAutoPinServiceObserver (store: Store) {
    const braveWalletAutoPinServiceObserverReceiver = new BraveWallet.WalletAutoPinServiceObserverReceiver({
      onAutoPinStatusChanged: function (enabled) {
        store.dispatch(WalletPageActions.updateAutoPinEnabled(
          enabled
        ))
      }
    })
    this.braveWalletAutoPinService.addObserver(braveWalletAutoPinServiceObserverReceiver.$.bindNewPipeAndPassRemote())
  }
}

export default WalletApiProxy
