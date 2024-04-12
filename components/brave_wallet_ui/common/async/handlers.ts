// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import getAPIProxy from './bridge'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import { walletApi } from '../slices/api.slice'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

handler.on(
  WalletActions.refreshNetworksAndTokens.type,
  async (store: Store) => {
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

handler.on(WalletActions.initialize.type, async (store) => {
  // Initialize active origin state.
  const braveWalletService = getAPIProxy().braveWalletService
  const { originInfo } = await braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(originInfo))
  store.dispatch(WalletActions.refreshAll())
})

handler.on(WalletActions.walletReset.type, async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.type, async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  store.dispatch(WalletActions.refreshAll())
})

handler.on(WalletActions.unlocked.type, async (store) => {
  store.dispatch(WalletActions.refreshAll())
})

handler.on(WalletActions.backedUp.type, async (store) => {
  store.dispatch(WalletActions.refreshAll())
})

handler.on(WalletActions.defaultBaseCurrencyChanged.type, async (store) => {
  store.dispatch(WalletActions.refreshAll())
})

handler.on(
  WalletActions.defaultBaseCryptocurrencyChanged.type,
  async (store) => {
    store.dispatch(WalletActions.refreshAll())
  }
)

handler.on(WalletActions.refreshAll.type, async (store: Store) => {
  const { keyringService, walletHandler } = getAPIProxy()
  const { walletInfo } = await walletHandler.getWalletInfo()
  if (!walletInfo.isWalletLocked) {
    keyringService.notifyUserInteraction()
  }
  interactionNotifier.beginWatchingForInteraction(
    50000,
    walletInfo.isWalletLocked,
    async () => {
      keyringService.notifyUserInteraction()
    }
  )
  const braveWalletService = getAPIProxy().braveWalletService
  store.dispatch(
    walletApi.util.invalidateTags([
      'WalletInfo',
      'DefaultFiatCurrency',
      'ConnectedAccounts',
      'DefaultEthWallet',
      'DefaultSolWallet',
      'IsMetaMaskInstalled'
    ])
  )
  // Fetch Balances and Prices
  if (!walletInfo.isWalletLocked && walletInfo.isWalletCreated) {
    // refresh networks registry & selected network
    await store
      .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
      .unwrap()
    store.dispatch(walletApi.endpoints.invalidateUserTokensRegistry.initiate())
    braveWalletService.discoverAssetsOnAllSupportedChains(false)
  }
})

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
