/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createSlice, PayloadAction } from '@reduxjs/toolkit'
import { AccountModalTypes, BraveWallet } from '../../constants/types'

export interface AccountsTabState {
  selectedAccount: BraveWallet.AccountInfo | undefined
  showAccountModal: boolean
  accountModalType: AccountModalTypes
  accountToRemove:
    | {
        accountId: BraveWallet.AccountId
        name: string
      }
    | undefined
}

const defaultState: AccountsTabState = {
  selectedAccount: undefined,
  showAccountModal: false,
  accountModalType: 'deposit',
  accountToRemove: undefined,
}

const accountsTabSlice = createSlice({
  name: 'accountsTab',
  initialState: defaultState,
  reducers: {
    setSelectedAccount(
      state,
      action: PayloadAction<BraveWallet.AccountInfo | undefined>,
    ) {
      state.selectedAccount = action.payload
    },
    setShowAccountModal(state, action: PayloadAction<boolean>) {
      state.showAccountModal = action.payload
    },
    setAccountModalType(state, action: PayloadAction<AccountModalTypes>) {
      state.accountModalType = action.payload
    },
    setAccountToRemove(
      state,
      action: PayloadAction<
        | {
            accountId: BraveWallet.AccountId
            name: string
          }
        | undefined
      >,
    ) {
      state.accountToRemove = action.payload
    },
  },
})

export const AccountsTabActions = accountsTabSlice.actions

export const createAccountsTabReducer = (initialState: AccountsTabState) => {
  return (
    state: AccountsTabState = initialState,
    action: Parameters<typeof accountsTabSlice.reducer>[1],
  ) => {
    return accountsTabSlice.reducer(state, action)
  }
}

export default accountsTabSlice.reducer
