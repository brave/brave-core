// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types & constants
import { BraveWallet } from '../constants/types'
import { Store } from './async/types'

// redux
import * as WalletActions from './actions/wallet_actions'
import { walletApi } from './slices/api.slice'

// utils
import { makeSerializableTransaction } from '../utils/model-serialization-utils'
import { getCoinFromTxDataUnion } from '../utils/network-utils'
import { getHasPendingRequests } from '../utils/api-utils'

export function makeBraveWalletServiceTokenObserver(store: Store) {
  const braveWalletServiceTokenObserverReceiver =
    new BraveWallet.BraveWalletServiceTokenObserverReceiver({
      onTokenAdded(token) {
        store.dispatch(
          walletApi.endpoints.invalidateUserTokensRegistry.initiate()
        )
        store.dispatch(WalletActions.refreshNetworksAndTokens())
        // re-parse transactions with new coins list
        store.dispatch(
          walletApi.endpoints.invalidateTransactionsCache.initiate()
        )
      },
      onTokenRemoved(token) {
        store.dispatch(
          walletApi.endpoints.invalidateUserTokensRegistry.initiate()
        )
        store.dispatch(WalletActions.refreshNetworksAndTokens())
        // re-parse transactions with new coins list
        store.dispatch(
          walletApi.endpoints.invalidateTransactionsCache.initiate()
        )
      }
    })
  return braveWalletServiceTokenObserverReceiver
}

export function makeJsonRpcServiceObserver(store: Store) {
  const jsonRpcServiceObserverReceiver =
    new BraveWallet.JsonRpcServiceObserverReceiver({
      chainChangedEvent: function (chainId, coin, origin) {
        store.dispatch(walletApi.endpoints.invalidateSelectedChain.initiate())
      },
      onAddEthereumChainRequestCompleted: function (chainId, error) {
        // update add/switch chain requests query data
        store.dispatch(
          walletApi.util.invalidateTags([
            'PendingAddChainRequests',
            'PendingSwitchChainRequests'
          ])
        )
      }
    })
  return jsonRpcServiceObserverReceiver
}

export function makeKeyringServiceObserver(store: Store) {
  const keyringServiceObserverReceiver =
    new BraveWallet.KeyringServiceObserverReceiver({
      walletCreated: function () {
        store.dispatch(WalletActions.walletCreated())
      },
      walletRestored: function () {
        store.dispatch(WalletActions.walletRestored())
      },
      walletReset: function () {
        store.dispatch(WalletActions.walletReset())
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
        store.dispatch(walletApi.endpoints.invalidateSelectedAccount.initiate())
      },
      accountsAdded: function () {
        store.dispatch(walletApi.endpoints.invalidateAccountInfos.initiate())
        store.dispatch(walletApi.endpoints.invalidateSelectedAccount.initiate())
      },
      autoLockMinutesChanged: function () {
        store.dispatch(WalletActions.autoLockMinutesChanged())
      },
      selectedWalletAccountChanged: function (
        account: BraveWallet.AccountInfo
      ) {
        store.dispatch(walletApi.endpoints.invalidateSelectedAccount.initiate())
      },
      selectedDappAccountChanged: function (
        coin: BraveWallet.CoinType,
        account: BraveWallet.AccountInfo | null
      ) {
        // TODO: Handle this event.
      }
    })
  return keyringServiceObserverReceiver
}

export function makeTxServiceObserver(store: Store) {
  const txServiceManagerObserverReceiver =
    new BraveWallet.TxServiceObserverReceiver({
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
            fromAccountId: txInfo.fromAccountId,
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
            getHasPendingRequests().then((hasPendingRequests) => {
              if (!hasPendingRequests) {
                store.dispatch(walletApi.endpoints.closePanelUI.initiate())
              }
            })
          }
        }
      },
      onTxServiceReset: function () {}
    })
  return txServiceManagerObserverReceiver
}

export function makeBraveWalletServiceObserver(store: Store) {
  let lastKnownActiveOrigin: BraveWallet.OriginInfo
  const braveWalletServiceObserverReceiver =
    new BraveWallet.BraveWalletServiceObserverReceiver({
      onActiveOriginChanged: function (originInfo) {
        // check that the origin has changed from the stored values
        // in any way before dispatching the update action
        if (
          lastKnownActiveOrigin &&
          lastKnownActiveOrigin.eTldPlusOne === originInfo.eTldPlusOne &&
          lastKnownActiveOrigin.originSpec === originInfo.originSpec
        ) {
          return
        }
        lastKnownActiveOrigin = originInfo
        store.dispatch(walletApi.util.invalidateTags(['ActiveOrigin']))
      },
      onDefaultEthereumWalletChanged: function (defaultWallet) {
        store.dispatch(
          walletApi.util.invalidateTags([
            'DefaultEthWallet',
            'IsMetaMaskInstalled'
          ])
        )
      },
      onDefaultSolanaWalletChanged: function (defaultWallet) {
        store.dispatch(walletApi.util.invalidateTags(['DefaultSolWallet']))
      },
      onDefaultBaseCurrencyChanged: function (currency) {
        store.dispatch(WalletActions.defaultBaseCurrencyChanged({ currency }))
      },
      onDefaultBaseCryptocurrencyChanged: function (cryptocurrency) {
        store.dispatch(
          WalletActions.defaultBaseCryptocurrencyChanged({ cryptocurrency })
        )
      },
      onNetworkListChanged: function () {
        // FIXME(onyb): Due to a bug, the OnNetworkListChanged event is fired
        // merely upon switching to a custom network.
        //
        // Skipping balances refresh for now, until the bug is fixed.
        store.dispatch(WalletActions.refreshNetworksAndTokens())
      },
      onDiscoverAssetsStarted: function () {
        store.dispatch(WalletActions.setAssetAutoDiscoveryCompleted(false))
      },
      onDiscoverAssetsCompleted: function () {
        store.dispatch(
          walletApi.endpoints.invalidateUserTokensRegistry.initiate()
        )
        store.dispatch(WalletActions.setAssetAutoDiscoveryCompleted(true))
      },
      onResetWallet: function () {}
    })
  return braveWalletServiceObserverReceiver
}
