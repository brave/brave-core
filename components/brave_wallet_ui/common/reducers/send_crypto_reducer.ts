/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer, createAction } from 'redux-act'
import { BraveWallet } from '../../constants/types'

export interface PendingCryptoSendState {
  selectedSendAsset?: BraveWallet.BlockchainToken
  toAddressOrUrl: string
  toAddress: string
  addressError?: string
  addressWarning?: string
  sendAmount: string
  showEnsOffchainLookupOptions: boolean
  ensOffchainLookupOptions?: BraveWallet.EnsOffchainLookupOptions
}

const defaultState: PendingCryptoSendState = {
  sendAmount: '',
  toAddress: '',
  addressError: undefined,
  addressWarning: undefined,
  selectedSendAsset: undefined,
  toAddressOrUrl: '',
  showEnsOffchainLookupOptions: false,
  ensOffchainLookupOptions: undefined
}

export const SendCryptoActions = {
  setSendAmount: createAction<string | undefined>('setSendAmount'),
  setToAddressOrUrl: createAction<string | undefined>('setToAddressOrUrl'),
  setAddressError: createAction<string | undefined>('setAddressError'),
  setToAddress: createAction<string | undefined>('setToAddress'),
  setShowEnsOffchainLookupOptions: createAction<boolean>('setShowEnsOffchainLookupOptions'),
  setEnsOffchainLookupOptions: createAction<BraveWallet.EnsOffchainLookupOptions | undefined>('setEnsOffchainLookupOptions'),
  setAddressWarning: createAction<string | undefined>('setAddressWarning'),
  selectSendAsset: createAction<BraveWallet.BlockchainToken | undefined>('selectSendAsset')
}

export const createSendCryptoReducer = (initialState: PendingCryptoSendState) => {
  const reducer = createReducer<PendingCryptoSendState>({}, initialState)

  reducer.on(SendCryptoActions.setSendAmount, (
    state: PendingCryptoSendState,
    payload: string
  ): PendingCryptoSendState => {
    return {
      ...state,
      sendAmount: payload
    }
  })

  reducer.on(SendCryptoActions.setToAddressOrUrl, (
    state: PendingCryptoSendState,
    payload: string
  ): PendingCryptoSendState => {
    return {
      ...state,
      toAddressOrUrl: payload
    }
  })

  reducer.on(SendCryptoActions.setAddressError, (
    state: PendingCryptoSendState,
    payload: string
  ): PendingCryptoSendState => {
    return {
      ...state,
      addressError: payload
    }
  })

  reducer.on(SendCryptoActions.setToAddress, (
    state: PendingCryptoSendState,
    payload: string
  ): PendingCryptoSendState => {
    return {
      ...state,
      toAddress: payload
    }
  })

  reducer.on(SendCryptoActions.setShowEnsOffchainLookupOptions, (
    state: PendingCryptoSendState,
    payload: boolean
  ): PendingCryptoSendState => {
    return {
      ...state,
      showEnsOffchainLookupOptions: payload
    }
  })

  reducer.on(SendCryptoActions.setEnsOffchainLookupOptions, (
    state: PendingCryptoSendState,
    payload: BraveWallet.EnsOffchainLookupOptions | undefined
  ): PendingCryptoSendState => {
    return {
      ...state,
      ensOffchainLookupOptions: payload
    }
  })

  reducer.on(SendCryptoActions.setAddressWarning, (
    state: PendingCryptoSendState,
    payload: string
  ): PendingCryptoSendState => {
    return {
      ...state,
      addressWarning: payload
    }
  })

  reducer.on(SendCryptoActions.selectSendAsset, (
    state: PendingCryptoSendState,
    payload: BraveWallet.BlockchainToken | undefined
  ): PendingCryptoSendState => {
    return {
      ...state,
      selectedSendAsset: payload
    }
  })

  return reducer
}

const sendCryptoReducer = createSendCryptoReducer(defaultState)

export default sendCryptoReducer
