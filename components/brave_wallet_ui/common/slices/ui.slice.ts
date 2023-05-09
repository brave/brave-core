// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import { BraveWallet, UIState } from '../../constants/types'
import { walletApi } from './api.slice'
import { SetTransactionProviderErrorType } from '../constants/action_types'

const defaultState: UIState = {
  selectedPendingTransactionId: undefined,
  transactionProviderErrorRegistry: {}
}

// slice
export const createUISlice = (initialState: UIState = defaultState) => {
  return createSlice({
    name: 'ui',
    initialState,
    reducers: {
      setPendingTransactionId: (
        state: UIState,
        { payload }: PayloadAction<string>
      ) => {
        state.selectedPendingTransactionId = payload
      },

      setTransactionProviderError: (
        state: UIState,
        { payload }: PayloadAction<SetTransactionProviderErrorType>
      ) => {
        state.transactionProviderErrorRegistry[payload.transaction.id] =
          payload.providerError
      },

    },
    extraReducers: (builder) => {
      builder.addMatcher(
        walletApi.endpoints.newUnapprovedTxAdded.matchFulfilled,
        (state, { payload }) => {
          // set the new transaction as the selected pending tx
          // if there is not one already
          if (!state.selectedPendingTransactionId) {
            state.selectedPendingTransactionId = payload.txId
          }
        }
      )

      builder.addMatcher(
        walletApi.endpoints.transactionStatusChanged.matchFulfilled,
        (state, { payload }) => {
          // set the new transaction as the selected pending tx
          // if there is not one already
          if (
            payload.status === BraveWallet.TransactionStatus.Unapproved &&
            !state.selectedPendingTransactionId
          ) {
            state.selectedPendingTransactionId = payload.txId
          }
        }
      )
    }
  })
}

export const createUIReducer = (initialState: UIState) => {
  return createUISlice(initialState).reducer
}

export const uiSlice = createUISlice()
export const uiReducer = uiSlice.reducer
export const UIActions = uiSlice.actions
export default uiReducer
