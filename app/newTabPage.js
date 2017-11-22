/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React from 'react'
import ReactDOM from 'react-dom'
import {Provider} from 'react-redux'
import {Store} from 'react-chrome-redux'
import NewTab from './containers/newTab'

chrome.storage.local.get('state', (obj) => {
  const store = new Store({
    portName: 'BRAVE'
  })
  store.ready().then(() => {
    const mountNode = document.querySelector('#root')
    ReactDOM.render(
      <Provider store={store}>
        <NewTab />
      </Provider>,
      mountNode
    )
  })
})
