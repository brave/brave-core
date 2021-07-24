/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { NetworkOptions } from '../../options/network-options'
import { createReducer } from 'redux-act'
import { NetworkOptionsType, WalletAccountType, WalletState } from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { InitializedPayloadType } from '../constants/action_types'

const defaultState: WalletState = {
  hasInitialized: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  selectedNetwork: NetworkOptions[0],
  accounts: [],
  walletAccountNames: [],
  transactions: []
}

const reducer = createReducer<WalletState>({}, defaultState)

reducer.on(WalletActions.initialized, (state: any, payload: InitializedPayloadType) => {
  const accounts = payload.accounts.map((address: string, idx: number) => {
    return {
      id: `${idx + 1}`,
      name: payload.walletAccountNames[idx],
      address,
      balance: 0,
      fiatBalance: '0',
      asset: 'eth',
      accountType: 'Primary'
    }
  })

  return {
    ...state,
    hasInitialized: true,
    isWalletCreated: payload.isWalletCreated,
    isWalletLocked: payload.isWalletLocked,
    favoriteApps: payload.favoriteApps,
    accounts,
    isWalletBackedUp: payload.isWalletBackedUp,
    walletAccountNames: payload.walletAccountNames,
    selectedAccount: accounts[0]
  }
})

reducer.on(WalletActions.hasIncorrectPassword, (state: any, payload: boolean) => {
  return {
    ...state,
    hasIncorrectPassword: payload
  }
})

reducer.on(WalletActions.selectAccount, (state: any, payload: WalletAccountType) => {
  return {
    ...state,
    selectedAccount: payload
  }
})

reducer.on(WalletActions.selectNetwork, (state: any, payload: NetworkOptionsType) => {
  return {
    ...state,
    selectedNetwork: payload
  }
})

export default reducer
