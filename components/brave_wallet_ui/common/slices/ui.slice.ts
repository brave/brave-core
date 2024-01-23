// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import { BraveWallet, UIState } from '../../constants/types'
import { walletApi } from './api.slice'
import { SetTransactionProviderErrorType } from '../constants/action_types'

// Utils
import { parseJSONFromLocalStorage } from '../../utils/local-storage-utils'

export const defaultUIState: UIState = {
  selectedPendingTransactionId: undefined,
  transactionProviderErrorRegistry: {},
  isPanel: false,
  collapsedPortfolioAccountIds: parseJSONFromLocalStorage(
    'COLLAPSED_PORTFOLIO_ACCOUNT_IDS',
    []
  ),
  collapsedPortfolioNetworkKeys: parseJSONFromLocalStorage(
    'COLLAPSED_PORTFOLIO_NETWORK_KEYS',
    []
  )
}

// slice
export const createUISlice = (initialState: UIState = defaultUIState) => {
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
        state.transactionProviderErrorRegistry[payload.transactionId] =
          payload.providerError
      },

      setCollapsedPortfolioAccountIds(
        state: UIState,
        { payload }: PayloadAction<string[]>
      ) {
        state.collapsedPortfolioAccountIds = payload
      },

      setCollapsedPortfolioNetworkKeys(
        state: UIState,
        { payload }: PayloadAction<string[]>
      ) {
        state.collapsedPortfolioNetworkKeys = payload
      }
    },
    extraReducers: (builder) => {
      builder.addMatcher(
        walletApi.endpoints.getTransactions.matchFulfilled,
        (state, { payload }) => {
          // set the the first pending transaction as the selected pending tx
          // if there is not one already
          if (!state.selectedPendingTransactionId) {
            const firstPendingTx = payload.find(
              (tx) => tx.txStatus === BraveWallet.TransactionStatus.Unapproved
            )
            if (firstPendingTx) {
              state.selectedPendingTransactionId = firstPendingTx.id
            }
          }
        }
      )

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
          // set the updated transaction as the selected pending tx
          // if there is not one already
          if (
            !state.selectedPendingTransactionId &&
            payload.status === BraveWallet.TransactionStatus.Unapproved
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
