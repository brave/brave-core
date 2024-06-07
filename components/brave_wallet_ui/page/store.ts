/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { configureStore } from '@reduxjs/toolkit'

// async handlers
import walletAsyncHandler from '../common/async/handlers'

// api
import getWalletPageApiProxy from './wallet_page_api_proxy'
import { walletApi } from '../common/slices/api.slice'

// reducers
import walletReducer from '../common/slices/wallet.slice'
import accountsTabReducer from './reducers/accounts-tab-reducer'
import pageReducer from './reducers/page_reducer'
import uiReducer from '../common/slices/ui.slice'

// utils
import {
  makeBraveWalletServiceObserver,
  makeBraveWalletServiceTokenObserver,
  makeJsonRpcServiceObserver,
  makeKeyringServiceObserver,
  makeTxServiceObserver
} from '../common/wallet_api_proxy_observers'

export const store = configureStore({
  reducer: {
    page: pageReducer,
    wallet: walletReducer,
    accountsTab: accountsTabReducer,
    ui: uiReducer,
    [walletApi.reducerPath]: walletApi.reducer
  },
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware({
      serializableCheck: false
    }).concat(walletAsyncHandler, walletApi.middleware)
})

export type WalletPageRootStore = typeof store
export type RootStoreState = ReturnType<typeof store.getState>

const proxy = getWalletPageApiProxy()
proxy.addJsonRpcServiceObserver(makeJsonRpcServiceObserver(store))
proxy.addKeyringServiceObserver(makeKeyringServiceObserver(store))
proxy.addTxServiceObserver(makeTxServiceObserver(store))
proxy.addBraveWalletServiceObserver(makeBraveWalletServiceObserver(store))
proxy.addBraveWalletServiceTokenObserver(
  makeBraveWalletServiceTokenObserver(store)
)

export const walletPageApiProxy = proxy

export default store
