/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { configureStore } from '@reduxjs/toolkit'

// api
import getWalletPanelApiProxy from './wallet_panel_api_proxy'

// reducers
import { walletApi } from '../common/slices/api.slice'
import walletReducer from '../common/slices/wallet.slice'
import pageReducer from '../page/reducers/page_reducer'
import { panelReducer } from '../common/slices/panel.slice'
import accountsTabReducer from '../page/reducers/accounts-tab-reducer'
import { uiReducer, defaultUIState } from '../common/slices/ui.slice'

// utils
import {
  makeJsonRpcServiceObserver,
  makeKeyringServiceObserver,
  makeTxServiceObserver,
  makeBraveWalletServiceObserver,
  makeBraveWalletServiceTokenObserver
} from '../common/wallet_api_proxy_observers'

const store = configureStore({
  reducer: {
    panel: panelReducer,
    page: pageReducer,
    accountsTab: accountsTabReducer,
    wallet: walletReducer,
    ui: uiReducer,
    [walletApi.reducerPath]: walletApi.reducer
  },
  preloadedState: {
    ui: {
      ...defaultUIState,
      isPanel: true
    }
  },
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware({
      serializableCheck: false
    }).concat(walletApi.middleware)
})

export type PanelStore = typeof store
export type PanelStoreState = ReturnType<typeof store.getState>
export type WalletPanelAppDispatch = typeof store.dispatch

const proxy = getWalletPanelApiProxy()
proxy.addJsonRpcServiceObserver(makeJsonRpcServiceObserver(store))
proxy.addKeyringServiceObserver(makeKeyringServiceObserver(store))
proxy.addTxServiceObserver(makeTxServiceObserver(store))
proxy.addBraveWalletServiceObserver(makeBraveWalletServiceObserver(store))
proxy.addBraveWalletServiceTokenObserver(
  makeBraveWalletServiceTokenObserver(store)
)

export const walletPanelApiProxy = proxy

export default store
