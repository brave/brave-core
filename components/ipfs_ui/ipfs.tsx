/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

// Components
import App from './components/app'

// Utils
import store from './store'
import * as ipfsActions from './actions/ipfs_actions'

window.cr.define('ipfs', function () {
  'use strict'


  function getConnectedPeers() {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getConnectedPeers()
  }

  function initialize () {
    getConnectedPeers()
    render(
      <Provider store={store}>
        <App />
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function onGetConnectedPeers (peerCount: number) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetConnectedPeers(peerCount)
  }

  return {
    initialize,
    onGetConnectedPeers
  }
})

document.addEventListener('DOMContentLoaded', window.ipfs.initialize)
