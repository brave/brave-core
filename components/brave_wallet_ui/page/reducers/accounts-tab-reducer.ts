/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer, createAction } from 'redux-act'
import { WalletAccountType, AccountModalTypes, BraveWallet } from '../../constants/types'

export interface AccountsTabState {
  selectedAccount: WalletAccountType | undefined
  showAccountModal: boolean
  accountModalType: AccountModalTypes
  accountToRemove: {
    address: string
    hardware: boolean
    coin: BraveWallet.CoinType
    name: string
  } | undefined
}

const defaultState: AccountsTabState = {
  selectedAccount: undefined,
  showAccountModal: false,
  accountModalType: 'deposit',
  accountToRemove: undefined
}

export const AccountsTabActions = {
  setSelectedAccount: createAction<WalletAccountType | undefined>('setSelectedAccount'),
  setShowAccountModal: createAction<boolean>('setShowAccountModal'),
  setAccountModalType: createAction<AccountModalTypes>('setAccountModalType'),
  setAccountToRemove: createAction<{
    address: string
    hardware: boolean
    coin: BraveWallet.CoinType
    name: string
  } | undefined>('setAccountToRemove')
}

export const createAccountsTabReducer = (initialState: AccountsTabState) => {
  const reducer = createReducer<AccountsTabState>({}, initialState)

  reducer.on(AccountsTabActions.setSelectedAccount, (
    state: AccountsTabState,
    payload: WalletAccountType | undefined
  ): AccountsTabState => {
    return {
      ...state,
      selectedAccount: payload
    }
  })

  reducer.on(AccountsTabActions.setShowAccountModal, (
    state: AccountsTabState,
    payload: boolean
  ): AccountsTabState => {
    return {
      ...state,
      showAccountModal: payload
    }
  })

  reducer.on(AccountsTabActions.setAccountModalType, (
    state: AccountsTabState,
    payload: AccountModalTypes
  ): AccountsTabState => {
    return {
      ...state,
      accountModalType: payload
    }
  })

  reducer.on(AccountsTabActions.setAccountToRemove, (
    state: AccountsTabState,
    payload: {
      address: string
      hardware: boolean
      coin: BraveWallet.CoinType
      name: string
    } | undefined
  ): AccountsTabState => {
    return {
      ...state,
      accountToRemove: payload
    }
  })

  return reducer
}

const accountsTabReducer = createAccountsTabReducer(defaultState)

export default accountsTabReducer
