// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { ListenerEffectAPI } from '@reduxjs/toolkit'

import * as WalletActions from '../actions/wallet_actions'
import getAPIProxy from './bridge'
import { interactionNotifier } from './interactionNotifier'
import { walletApi } from '../slices/api.slice'
import { startAppListening } from './listenerMiddleware'
import type { State, Dispatch } from './types'

type ListenerApi = ListenerEffectAPI<State, Dispatch>

const onInteractionInterval = () => {
  getAPIProxy().keyringService.notifyUserInteraction()
}

async function refreshWalletInfo(listenerApi: ListenerApi) {
  const apiProxy = getAPIProxy()

  const { walletInfo } = await apiProxy.walletHandler.getWalletInfo()
  const { allAccounts } = await apiProxy.keyringService.getAllAccounts()
  listenerApi.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  listenerApi.dispatch(WalletActions.refreshAll())

  // refresh networks registry & selected network
  await listenerApi
    .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
    .unwrap()

  listenerApi.dispatch(
    walletApi.util.invalidateTags([
      'ConnectedAccounts',
      'DefaultEthWallet',
      'DefaultSolWallet',
      'DefaultAdaWallet',
      'IsMetaMaskInstalled',
      // So backup warning banner updates when wallet is restored natively (e.g. iOS)
      'IsWalletBackedUp',
    ]),
  )
}

startAppListening({
  actionCreator: WalletActions.refreshNetworksAndTokens,
  effect: async (_, listenerApi) => {
    // refresh networks registry & selected network
    listenerApi.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(true))
    await listenerApi
      .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
      .unwrap()
    await listenerApi
      .dispatch(walletApi.endpoints.invalidateUserTokensRegistry.initiate())
      .unwrap()
    listenerApi.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(false))
  },
})

startAppListening({
  actionCreator: WalletActions.initialize,
  effect: async (_, listenerApi) => {
    const { walletHandler, keyringService } = getAPIProxy()
    const { walletInfo } = await walletHandler.getWalletInfo()
    const { isWalletLocked } = walletInfo
    const { allAccounts } = await keyringService.getAllAccounts()
    if (!isWalletLocked) {
      keyringService.notifyUserInteraction()
    }
    interactionNotifier.beginWatchingForInteraction(
      50000,
      isWalletLocked,
      onInteractionInterval,
    )
    listenerApi.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))

    // iOS: refetch backup status once on load when not backed up so the Backup
    // Wallet Warning banner updates after native restore (web UI restore
    // mutation never runs there).
    const state = listenerApi.getState()
    if (state.ui?.isIOS && !walletInfo.isWalletBackedUp) {
      listenerApi.dispatch(walletApi.util.invalidateTags(['IsWalletBackedUp']))
    }
  },
})

startAppListening({
  actionCreator: WalletActions.walletCreated,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.walletRestored,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.walletReset,
  effect: async () => {
    window.location.reload()
  },
})

startAppListening({
  actionCreator: WalletActions.locked,
  effect: async (_, listenerApi) => {
    interactionNotifier.stopWatchingForInteraction()
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.unlocked,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
    const { keyringService } = getAPIProxy()
    keyringService.notifyUserInteraction()
    interactionNotifier.beginWatchingForInteraction(
      50000,
      false,
      onInteractionInterval,
    )
  },
})

startAppListening({
  actionCreator: WalletActions.backedUp,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.defaultBaseCurrencyChanged,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.defaultBaseCryptocurrencyChanged,
  effect: async (_, listenerApi) => {
    await refreshWalletInfo(listenerApi)
  },
})

startAppListening({
  actionCreator: WalletActions.refreshAll,
  effect: async (_, listenerApi) => {
    const { braveWalletService, walletHandler } = getAPIProxy()
    const { walletInfo } = await walletHandler.getWalletInfo()
    const { isWalletCreated, isWalletLocked } = walletInfo
    listenerApi.dispatch(walletApi.util.invalidateTags(['DefaultFiatCurrency']))
    // Fetch Balances and Prices
    if (!isWalletLocked && isWalletCreated) {
      // refresh networks registry & selected network
      await listenerApi
        .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
        .unwrap()
      listenerApi.dispatch(
        walletApi.endpoints.invalidateUserTokensRegistry.initiate(),
      )
      await braveWalletService.discoverAssetsOnAllSupportedChains(false)
    }
  },
})

startAppListening({
  actionCreator: WalletActions.refreshBalancesAndPriceHistory,
  effect: async (_, listenerApi) => {
    listenerApi.dispatch(
      walletApi.util.invalidateTags([
        'TokenBalances',
        'TokenBalancesForChainId',
        'AccountTokenCurrentBalance',
        'HardwareAccountDiscoveryBalance',
      ]),
    )
  },
})
