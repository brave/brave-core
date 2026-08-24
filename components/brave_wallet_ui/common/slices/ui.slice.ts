// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import { BraveWallet, UIState } from '../../constants/types'
import { walletApi } from './api.slice'
import { SetTransactionProviderErrorType } from '../constants/action_types'
import {
  setSelectedTransactionId,
  setSubmittingTransaction,
} from './ui_tx_actions'

export const defaultUIState: UIState = {
  selectedPendingTransactionId: undefined,
  transactionProviderErrorRegistry: {},
  isPanel: false,
  isSidePanel: false,
  isMobile: false,
  isIOS: false,
  selectedTransactionId: undefined,
  submittingTransaction: undefined,
}

// slice
export const createUISlice = (initialState: UIState = defaultUIState) => {
  return createSlice({
    name: 'ui',
    initialState,
    reducers: {
      setPendingTransactionId: (
        state: UIState,
        { payload }: PayloadAction<string>,
      ) => {
        state.selectedPendingTransactionId = payload
      },

      setTransactionProviderError: (
        state: UIState,
        { payload }: PayloadAction<SetTransactionProviderErrorType>,
      ) => {
        state.transactionProviderErrorRegistry[payload.transactionId] =
          payload.providerError
      },
    },
    extraReducers: (builder) => {
      builder.addCase(setSelectedTransactionId, (state, { payload }) => {
        state.selectedTransactionId = payload
      })

      builder.addCase(setSubmittingTransaction, (state, { payload }) => {
        state.submittingTransaction = payload
      })

      builder.addMatcher(
        walletApi.endpoints.getTransactions.matchFulfilled,
        (state, { payload }) => {
          // set the the first pending transaction as the selected pending tx
          // if there is not one already
          if (!state.selectedPendingTransactionId) {
            const firstPendingTx = payload.find(
              (tx) => tx.txStatus === BraveWallet.TransactionStatus.Unapproved,
            )
            if (firstPendingTx) {
              state.selectedPendingTransactionId = firstPendingTx.id
            }
          }
        },
      )

      builder.addMatcher(
        walletApi.endpoints.newUnapprovedTxAdded.matchFulfilled,
        (state, { payload }) => {
          // set the new transaction as the selected pending tx
          // if there is not one already
          if (!state.selectedPendingTransactionId) {
            state.selectedPendingTransactionId = payload.txId
          }
        },
      )

      builder.addMatcher(
        walletApi.endpoints.transactionStatusChanged.matchFulfilled,
        (state, { payload }) => {
          // set the updated transaction as the selected pending tx
          // if there is not one already
          if (
            !state.selectedPendingTransactionId
            && payload.status === BraveWallet.TransactionStatus.Unapproved
          ) {
            state.selectedPendingTransactionId = payload.txId
          }
        },
      )
    },
  })
}

export const createUIReducer = (initialState: UIState) => {
  return createUISlice(initialState).reducer
}

export const uiSlice = createUISlice()
export const uiReducer = uiSlice.reducer
export const UIActions = {
  ...uiSlice.actions,
  setSelectedTransactionId,
  setSubmittingTransaction,
}
export default uiReducer
