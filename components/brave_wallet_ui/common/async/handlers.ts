// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import { WalletState, RefreshOpts } from '../../constants/types'

// Utils
import getAPIProxy from './bridge'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import { walletApi } from '../slices/api.slice'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

function getWalletState(store: Store): WalletState {
  return store.getState().wallet
}

async function refreshWalletInfo(store: Store, payload: RefreshOpts = {}) {
  const apiProxy = getAPIProxy()

  const { walletInfo } = await apiProxy.walletHandler.getWalletInfo()
  const { allAccounts } = await apiProxy.keyringService.getAllAccounts()
  store.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  store.dispatch(WalletActions.refreshAll(payload))

  // refresh networks registry & selected network
  await store
    .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
    .unwrap()

  store.dispatch(
    walletApi.util.invalidateTags([
      'ConnectedAccounts',
      'DefaultEthWallet',
      'DefaultSolWallet',
      'IsMetaMaskInstalled'
    ])
  )
}

handler.on(
  WalletActions.refreshNetworksAndTokens.type,
  async (store: Store, payload: RefreshOpts) => {
    // refresh networks registry & selected network
    store.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(true))
    await store
      .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
      .unwrap()
    await store
      .dispatch(walletApi.endpoints.invalidateUserTokensRegistry.initiate())
      .unwrap()
    store.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(false))
  }
)

handler.on(
  WalletActions.initialize.type,
  async (store, payload: RefreshOpts) => {
    // Initialize active origin state.
    const braveWalletService = getAPIProxy().braveWalletService
    const { originInfo } = await braveWalletService.getActiveOrigin()
    store.dispatch(WalletActions.activeOriginChanged(originInfo))
    await refreshWalletInfo(store, payload)
  }
)

handler.on(WalletActions.walletCreated.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.walletRestored.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.walletReset.type, async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.type, async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.type, async (store) => {
  await refreshWalletInfo(store, {
    skipBalancesRefresh: true
  })
})

handler.on(WalletActions.backedUp.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(
  WalletActions.defaultBaseCryptocurrencyChanged.type,
  async (store) => {
    await refreshWalletInfo(store)
  }
)

handler.on(
  WalletActions.refreshAll.type,
  async (store: Store, payload: RefreshOpts) => {
    const keyringService = getAPIProxy().keyringService
    const state = getWalletState(store)
    if (!state.isWalletLocked) {
      keyringService.notifyUserInteraction()
    }
    interactionNotifier.beginWatchingForInteraction(
      50000,
      state.isWalletLocked,
      async () => {
        keyringService.notifyUserInteraction()
      }
    )
    const braveWalletService = getAPIProxy().braveWalletService
    store.dispatch(walletApi.util.invalidateTags(['DefaultFiatCurrency']))
    // Fetch Balances and Prices
    if (!state.isWalletLocked && state.isWalletCreated) {
      // refresh networks registry & selected network
      await store
        .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
        .unwrap()
      store.dispatch(
        walletApi.endpoints.invalidateUserTokensRegistry.initiate()
      )
      await braveWalletService.discoverAssetsOnAllSupportedChains(false)
    }
  }
)

handler.on(
  WalletActions.refreshBalancesAndPriceHistory.type,
  async (store: Store) => {
    store.dispatch(
      walletApi.util.invalidateTags([
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance',
        'HardwareAccountDiscoveryBalance'
      ])
    )
  }
)

export default handler.middleware
