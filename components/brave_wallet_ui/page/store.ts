/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createStore, applyMiddleware } from 'redux'

import reducers from './reducers'
import walletPageAsyncHandler from './async/wallet_page_async_handler'
import walletAsyncHandler from '../common/async/wallet_async_handler'
import apiProxy from './wallet_page_api_proxy.js'

const store = createStore(
    reducers,
    applyMiddleware(walletAsyncHandler, walletPageAsyncHandler)
)

apiProxy.getInstance().addEthJsonRpcControllerObserver(store)
apiProxy.getInstance().addKeyringControllerObserver(store)
apiProxy.getInstance().addEthTxControllerObserverObserver(store)

export default store
