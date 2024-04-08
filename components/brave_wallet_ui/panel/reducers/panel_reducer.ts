/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'

// types
import { BraveWallet, PanelState, PanelTypes } from '../../constants/types'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'
import * as PanelActions from '../actions/wallet_panel_actions'
import { ShowConnectToSitePayload } from '../constants/action_types'

// options
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

// utils
import { isValidPanelNavigationOption } from '../../options/nav-options'

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
  signMessageData: [],
  hardwareWalletCode: undefined,
  selectedTransactionId: undefined,
  signMessageErrorData: []
}

export const createPanelReducer = (initialState: PanelState) => {
  const reducer = createReducer<PanelState>({}, initialState)
  reducer.on(
    PanelActions.navigateTo.type,
    (state: PanelState, selectedPanel: PanelTypes) => {
      return {
        ...state,
        selectedPanel
      }
    }
  )

  reducer.on(
    PanelActions.showConnectToSite.type,
    (state: any, payload: ShowConnectToSitePayload) => {
      return {
        ...state,
        connectToSiteOrigin: payload.originInfo,
        connectingAccounts: payload.accounts
      }
    }
  )

  reducer.on(
    PanelActions.signMessage.type,
    (state, payload: BraveWallet.SignMessageRequest[]) => {
      return {
        ...state,
        signMessageData: payload
      }
    }
  )

  reducer.on(
    PanelActions.setHardwareWalletInteractionError.type,
    (state: any, payload?: HardwareWalletResponseCodeType) => {
      return {
        ...state,
        hardwareWalletCode: payload
      }
    }
  )

  reducer.on(
    PanelActions.setSelectedTransactionId.type,
    (state: PanelState, payload: string | undefined): PanelState => {
      return {
        ...state,
        selectedTransactionId: payload
      }
    }
  )

  reducer.on(
    PanelActions.signMessageError.type,
    (
      state: PanelState,
      payload: BraveWallet.SignMessageError[]
    ): PanelState => {
      return {
        ...state,
        signMessageErrorData: payload
      }
    }
  )
  return reducer
}

const reducer = createPanelReducer(defaultState)
export const panelReducer = reducer
export default reducer
