/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import {
  BraveWallet,
  HardwareWalletResponseCodeType,
  PanelState,
  PanelTypes,
  TransactionInfoLookup,
} from '../../constants/types'
import { ShowConnectToSitePayload } from '../constants/action_types'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'
import { isValidPanelNavigationOption } from '../../options/nav-options'

const defaultOriginInfo: BraveWallet.OriginInfo = {
  originSpec: '',
  eTldPlusOne: '',
}

const persistedSelectedPanelType = window.localStorage.getItem(
  LOCAL_STORAGE_KEYS.CURRENT_PANEL,
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
  selectedTransactionId: undefined,
}

export const createPanelSlice = (initialState: PanelState = defaultState) => {
  return createSlice({
    name: 'panel',
    initialState,
    reducers: {
      navigateTo(state, action: PayloadAction<PanelTypes>) {
        state.selectedPanel = action.payload
      },
      showConnectToSite(
        state,
        action: PayloadAction<ShowConnectToSitePayload>,
      ) {
        state.connectToSiteOrigin = action.payload.originInfo
        state.connectingAccounts = action.payload.accounts
      },
      setHardwareWalletInteractionError(
        state,
        action: PayloadAction<HardwareWalletResponseCodeType | undefined>,
      ) {
        state.hardwareWalletCode = action.payload
      },
      setSelectedTransactionId(
        state,
        action: PayloadAction<TransactionInfoLookup | undefined>,
      ) {
        state.selectedTransactionId = action.payload
      },
    },
  })
}

const panelSlice = createPanelSlice()

export const PanelSliceActions = panelSlice.actions
export const panelReducer = panelSlice.reducer
export default panelReducer
