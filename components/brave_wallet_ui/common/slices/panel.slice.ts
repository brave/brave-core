// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

// types
import type {
  BraveWallet,
  HardwareWalletResponseCodeType,
  PanelState,
  PanelTypes,
  TransactionInfoLookup
} from '../../constants/types'
import { ShowConnectToSitePayload } from '../../panel/constants/action_types'

// utils
import { isValidPanelNavigationOption } from '../../options/nav-options'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'

const defaultOriginInfo: BraveWallet.OriginInfo = {
  originSpec: '',
  eTldPlusOne: ''
}

const persistedSelectedPanelType = window.localStorage.getItem(
  LOCAL_STORAGE_KEYS.CURRENT_PANEL
) as PanelTypes
const selectedPanel = isValidPanelNavigationOption(persistedSelectedPanelType)
  ? persistedSelectedPanelType
  : 'main'

const defaultState: PanelState = {
  hasInitialized: false,
  connectToSiteOrigin: defaultOriginInfo,
  selectedPanel,
  connectingAccounts: [],
  hardwareWalletCode: undefined,
  selectedTransactionId: undefined
}

export const createPanelSlice = (initialState: PanelState = defaultState) => {
  return createSlice({
    initialState,
    name: 'panel',
    reducers: {
      setSelectedPanel: (state, { payload }: PayloadAction<PanelTypes>) => {
        state.selectedPanel = payload

        if (payload === 'main') {
          // clear hardware wallet interaction error
          state.hardwareWalletCode = undefined
        }
      },

      setSiteConnectionPayload: (
        state,
        { payload }: PayloadAction<ShowConnectToSitePayload>
      ) => {
        state.connectToSiteOrigin = payload.originInfo
        state.connectingAccounts = payload.accounts
      },

      setHardwareWalletInteractionError: (
        state,
        { payload }: PayloadAction<HardwareWalletResponseCodeType | undefined>
      ) => {
        state.hardwareWalletCode = payload
      },

      setSelectedTransactionId: (
        state,
        { payload }: PayloadAction<TransactionInfoLookup | undefined>
      ) => {
        state.selectedTransactionId = payload
      }
    }
  })
}

export const createPanelReducer = (initialState: PanelState) => {
  return createPanelSlice(initialState).reducer
}

export const panelSlice = createPanelSlice()
export const panelReducer = panelSlice.reducer
export const PanelActions = panelSlice.actions
export default panelReducer
