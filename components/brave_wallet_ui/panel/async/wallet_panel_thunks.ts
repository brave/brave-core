// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAsyncThunk } from '@reduxjs/toolkit'

// types
import {
  BraveWallet,
  HardwareVendor,
  PanelTypes,
  ReduxStoreState
} from '../../constants/types'

// Core
import getWalletPanelApiProxy from '../wallet_panel_api_proxy'

// actions
import { ShowConnectToSitePayload } from '../constants/action_types'
import { PanelActions } from '../../common/slices/panel.slice'

// Hardware
import { cancelHardwareOperation } from '../../common/async/hardware'

// utils
import { storeCurrentAndPreviousPanel } from '../../utils/local-storage-utils'

export const navigateTo = createAsyncThunk(
  'navigateTo',
  async (payload: PanelTypes, store) => {
    store.dispatch(PanelActions.setSelectedPanel(payload))

    // persist navigation state
    const selectedPanel = (store.getState() as ReduxStoreState).panel
      ?.selectedPanel
    // navigating away from the current panel, store the last known location
    storeCurrentAndPreviousPanel(payload, selectedPanel)
  }
)

export const navigateToMain = createAsyncThunk(
  'navigateToMain',
  async (payload, store) => {
    const { panelHandler } = getWalletPanelApiProxy()

    store.dispatch(navigateTo('main'))
    store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
    panelHandler.setCloseOnDeactivate(true)
    panelHandler.showUI()
  }
)

export const cancelConnectHardwareWallet = createAsyncThunk(
  'cancelConnectHardwareWallet',
  async (payload: BraveWallet.AccountInfo, store) => {
    if (payload.hardware) {
      // eslint-disable-next-line max-len
      // eslint-disable @typescript-eslint/no-unnecessary-type-assertion
      await cancelHardwareOperation(
        payload.hardware.vendor as HardwareVendor,
        payload.accountId.coin
      )
    }
    // Navigating to main panel view will unmount ConnectHardwareWalletPanel
    // and therefore forfeit connecting to the hardware wallet.
    await store.dispatch(navigateToMain())
  }
)

export const showConnectToSite = createAsyncThunk(
  'showConnectToSite',
  async (payload: ShowConnectToSitePayload, store) => {
    store.dispatch(navigateTo('connectWithSite'))
    store.dispatch(PanelActions.setSiteConnectionPayload(payload))
    getWalletPanelApiProxy().panelHandler.showUI()
  }
)

export const setCloseOnDeactivate = createAsyncThunk(
  'setCloseOnDeactivate',
  async (payload: boolean, store) => {
    getWalletPanelApiProxy().panelHandler.setCloseOnDeactivate(payload)
  }
)
