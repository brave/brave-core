/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createStore } from 'redux'
import { wrapStore } from 'webext-redux'

import reducers from './reducers'

const store = createStore(reducers)

wrapStore(store, {
  portName: 'WEBTORRENT'
})

export default store
