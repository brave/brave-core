/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers, configureStore } from '@reduxjs/toolkit'
import { setupListeners } from '@reduxjs/toolkit/query/react'
import { persistStore, DEFAULT_VERSION } from 'redux-persist'

// handlers
import walletPanelAsyncHandler from './async/wallet_panel_async_handler'
import walletAsyncHandler from '../common/async/handlers'

// api
import getWalletPanelApiProxy from './wallet_panel_api_proxy'

// reducers
import { walletApi } from '../common/slices/api.slice'
import { persistedWalletReducer } from '../common/slices/wallet.slice'
import pageReducer from '../page/reducers/page_reducer'
import accountsTabReducer from '../page/reducers/accounts-tab-reducer'
import { panelReducer } from './reducers/panel_reducer'
import {
  defaultUIState,
  persistedUiReducer
} from '../common/slices/ui.slice'

const combinedReducer = combineReducers({
  panel: panelReducer,
  page: pageReducer,
  accountsTab: accountsTabReducer,
  wallet: persistedWalletReducer,
  ui: persistedUiReducer,
  [walletApi.reducerPath]: walletApi.reducer
})

// utils
import { setApiProxyFetcher } from '../common/async/base-query-cache'

const store = configureStore({
  reducer: combinedReducer,
  preloadedState: {
    ui: {
      _persist: {
        rehydrated: false,
        version: DEFAULT_VERSION
      },
      ...defaultUIState,
      isPanel: true
    }
  },
  middleware: (getDefaultMiddleware) => getDefaultMiddleware({
    serializableCheck: false
  }).concat(
    walletAsyncHandler,
    walletPanelAsyncHandler,
    walletApi.middleware
  )
})

export type PanelRootState = ReturnType<typeof combinedReducer>
export type RootStoreState = PanelRootState

const proxy = getWalletPanelApiProxy()
proxy.addJsonRpcServiceObserver(store)
proxy.addKeyringServiceObserver(store)
proxy.addTxServiceObserver(store)
proxy.addBraveWalletServiceObserver(store)
proxy.addBraveWalletPinServiceObserver(store)
proxy.addBraveWalletAutoPinServiceObserver(store)

// use this proxy in the api slice of the store
setApiProxyFetcher(getWalletPanelApiProxy)

// enables refetchOnMount and refetchOnReconnect behaviors
// without this, cached data will not refresh when reloading the app
setupListeners(store.dispatch)

export const walletPanelApiProxy = proxy

export const persistor = persistStore(store)

export default store
