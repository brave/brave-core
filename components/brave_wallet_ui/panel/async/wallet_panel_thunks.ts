// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAsyncThunk } from '@reduxjs/toolkit'

// types
import { PanelTypes, ReduxStoreState } from '../../constants/types'

// Core
import getWalletPanelApiProxy from '../wallet_panel_api_proxy'

// actions
import { PanelActions } from '../../common/slices/panel.slice'

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

    if (payload === 'main') {
      store.dispatch(PanelActions.setHardwareWalletInteractionError(undefined))
      const { panelHandler } = getWalletPanelApiProxy()
      panelHandler.setCloseOnDeactivate(true)
      panelHandler.showUI()
    }
  }
)
