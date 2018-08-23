/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { applyMiddleware, createStore, Middleware } from 'redux'
import { wrapStore } from 'react-chrome-redux'
import { createLogger } from 'redux-logger'

import reducers from './reducers'

const logger = createLogger({
  collapsed: true
})

const getMiddleware = () => {
  const args: Middleware[] = []
  if (process.env.NODE_ENV === `development`) {
    args.push(logger)
  }

  return applyMiddleware(...args)
}

const initialState = {}
const store = createStore(reducers, initialState, getMiddleware())

wrapStore(store, {
  portName: 'WEBTORRENT'
})

export default store
