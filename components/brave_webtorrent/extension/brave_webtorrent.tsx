/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { Store } from 'react-chrome-redux'

// Components
import App from './components/app'

// Constants
import { TorrentsState } from './constants/webtorrentState'

const store: Store<TorrentsState> = new Store({
  portName: 'WEBTORRENT'
})

store.ready().then(
  () => {
    render(
      <Provider store={store}>
      <App />
      </Provider>,
      document.getElementById('root'))
  })
  .catch(() => {
    console.error('Problem mounting webtorrent')
  })
