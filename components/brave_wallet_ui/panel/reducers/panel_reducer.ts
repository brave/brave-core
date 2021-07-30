/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import { PanelState } from '../../constants/types'
import * as PanelActions from '../actions/wallet_panel_actions'

const defaultState: PanelState = {
  // TODO(bbondy): isConnected, connectedSiteOrigin, and accounts is just test
  // data to start with until the keyring controller is ready.
  isConnected: false,
  hasInitialized: false,
  connectedSiteOrigin: 'https://app.uniswap.org',
  selectedPanel: 'main',
  panelTitle: ''
}

const reducer = createReducer<PanelState>({}, defaultState)

reducer.on(PanelActions.navigateTo, (state: any, selectedPanel: string) => {
  let panelTitle = selectedPanel
  if (selectedPanel === 'networks') {
    // TODO(bbondy): This should be hooked up a localization label
    panelTitle = 'Select Network'
  }

  return {
    ...state,
    selectedPanel,
    panelTitle
  }
})

export default reducer
