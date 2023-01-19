/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { configureStore } from '@reduxjs/toolkit'

// handlers
import walletPanelAsyncHandler from './async/wallet_panel_async_handler'
import walletAsyncHandler from '../common/async/handlers'

// api
import getWalletPanelApiProxy from './wallet_panel_api_proxy'

// reducers
import { walletApi } from '../common/slices/api.slice'
import walletReducer from '../common/slices/wallet.slice'
import { panelReducer } from './reducers/panel_reducer'

const store = configureStore({
  reducer: {
    panel: panelReducer,
    wallet: walletReducer,
    [walletApi.reducerPath]: walletApi.reducer
  },
  middleware: (getDefaultMiddleware) => getDefaultMiddleware({
    serializableCheck: false
  }).concat(
    walletAsyncHandler,
    walletPanelAsyncHandler,
    walletApi.middleware
  )
})

const proxy = getWalletPanelApiProxy()
proxy.addJsonRpcServiceObserver(store)
proxy.addKeyringServiceObserver(store)
proxy.addTxServiceObserver(store)
proxy.addBraveWalletServiceObserver(store)

export const walletPanelApiProxy = proxy

export default store
