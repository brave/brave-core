/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createStore, applyMiddleware } from 'redux'
import thunk from 'redux-thunk'

import reducers from './reducers'
import walletPageAsyncHandler from './async/wallet_page_async_handler'
import walletAsyncHandler from '../common/async/handlers'
import getWalletPageApiProxy from './wallet_page_api_proxy'

const middlewares = [
  thunk,
  walletAsyncHandler,
  walletPageAsyncHandler
]

export const store = createStore(
  reducers,
  applyMiddleware(...middlewares)
)

const proxy = getWalletPageApiProxy()
proxy.addJsonRpcServiceObserver(store)
proxy.addKeyringServiceObserver(store)
proxy.addTxServiceObserver(store)
proxy.addBraveWalletServiceObserver(store)

export const walletPageApiProxy = proxy

export default store
