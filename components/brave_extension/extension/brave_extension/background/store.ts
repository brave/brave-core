/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { applyMiddleware, createStore, Middleware } from 'redux'
import { wrapStore } from 'webext-redux'
import { createLogger } from 'redux-logger'
import thunk from 'redux-thunk'

import reducer from './reducers'

const logger = createLogger({
  collapsed: true
})

const initialState = {}

const getMiddleware = () => {
  const args: Middleware[] = [thunk]

  if (process.env.NODE_ENV === 'shields_development') {
    args.push(logger)
  }

  return applyMiddleware(...args)
}

const store = createStore(
  reducer,
  initialState,
  getMiddleware()
)

wrapStore(store, {
  portName: 'BRAVE'
})

export default store
