/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import {
  BraveWallet,
  HardwareWalletOperation,
  HardwareWalletResponseCodeType,
  PanelState,
  PanelTypes,
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
  hardwareWalletOperation: undefined,
}

export const createPanelSlice = (initialState: PanelState = defaultState) => {
  return createSlice({
    name: 'panel',
    initialState,
    reducers: {
      setHasInitialized(state) {
        state.hasInitialized = true
      },
      navigateTo(state, action: PayloadAction<PanelTypes>) {
        // A different panel is a new session. Drop the bound operation so the
        // next hardware attempt can record the request that opened it.
        if (action.payload !== state.selectedPanel) {
          state.hardwareWalletOperation = undefined
        }
        state.selectedPanel = action.payload
      },
      setHardwareWalletOperation(
        state,
        action: PayloadAction<HardwareWalletOperation>,
      ) {
        // First writer wins. A retry must not retarget the panel.
        if (state.hardwareWalletOperation) {
          return
        }
        state.hardwareWalletOperation = action.payload
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
    },
  })
}

const panelSlice = createPanelSlice()

export const PanelSliceActions = panelSlice.actions
export const panelReducer = panelSlice.reducer
export default panelReducer
