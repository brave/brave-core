// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAsyncThunk } from '@reduxjs/toolkit'
import * as WalletActions from '../actions/wallet_actions'

// Types
import type { ReduxStoreState } from '../../constants/types'

// Redux
import { walletApi } from '../slices/api.slice'

// Utils
import getAPIProxy from './bridge'
import InteractionNotifier from './interactionNotifier'

const interactionNotifier = new InteractionNotifier()

export const refreshWalletInfo = createAsyncThunk(
  'refreshWalletInfo',
  async (_payload, { dispatch, getState }) => {
    const { wallet } = getState() as ReduxStoreState
    const { walletHandler, keyringService, braveWalletService } = getAPIProxy()

    if (!wallet.isWalletLocked) {
      keyringService.notifyUserInteraction()
    }

    interactionNotifier.beginWatchingForInteraction(
      50000,
      wallet.isWalletLocked,
      async () => {
        keyringService.notifyUserInteraction()
      }
    )

    const { walletInfo } = await walletHandler.getWalletInfo()
    const { allAccounts } = await keyringService.getAllAccounts()

    dispatch(WalletActions.initialized({ walletInfo, allAccounts }))

    dispatch(
      walletApi.util.invalidateTags([
        'DefaultFiatCurrency',
        'ConnectedAccounts',
        'DefaultEthWallet',
        'DefaultSolWallet',
        'IsMetaMaskInstalled'
      ])
    )

    // refresh networks registry & selected network
    await dispatch(walletApi.endpoints.refreshNetworkInfo.initiate()).unwrap()

    // Fetch Balances and Prices
    if (!wallet.isWalletLocked && wallet.isWalletCreated) {
      // refresh tokens registry
      dispatch(walletApi.endpoints.invalidateUserTokensRegistry.initiate())
      braveWalletService.discoverAssetsOnAllSupportedChains(false)
    }
  }
)

export const refreshNetworksAndTokens = createAsyncThunk(
  'refreshNetworksAndTokens',
  async (_payload, { dispatch }) => {
    // refresh networks registry & selected network
    dispatch(WalletActions.setIsRefreshingNetworksAndTokens(true))
    await dispatch(walletApi.endpoints.refreshNetworkInfo.initiate()).unwrap()
    await dispatch(
      walletApi.endpoints.invalidateUserTokensRegistry.initiate()
    ).unwrap()
    dispatch(WalletActions.setIsRefreshingNetworksAndTokens(false))
  }
)

export const locked = createAsyncThunk(
  'locked',
  async (_payload, { dispatch }) => {
    dispatch(refreshWalletInfo())
    interactionNotifier.stopWatchingForInteraction()
    dispatch(WalletActions.locked)
  }
)
