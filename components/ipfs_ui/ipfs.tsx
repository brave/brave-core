/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

// Components
import App from './components/app'
import { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'

// Utils
import store from './store'
import * as ipfsActions from './actions/ipfs_actions'

window.cr.define('ipfs', function () {
  'use strict'

  function getDaemonStatus () {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getDaemonStatus()
  }

  function getConnectedPeers () {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getConnectedPeers()
  }

  function getAddressesConfig () {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getAddressesConfig()
  }

  function getRepoStats () {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getRepoStats()
  }

  function getNodeInfo () {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.getNodeInfo()
  }

  function initialize () {
    getDaemonStatus()
    getConnectedPeers()
    getAddressesConfig()
    getRepoStats()
    getNodeInfo()
    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function onGetConnectedPeers (peerCount: number) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetConnectedPeers(peerCount)
  }

  function onGetAddressesConfig (addressesConfig: IPFS.AddressesConfig) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetAddressesConfig(addressesConfig)
  }

  function onGetDaemonStatus (daemonStatus: IPFS.DaemonStatus) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetDaemonStatus(daemonStatus)
  }

  function onGetRepoStats (repoStats: IPFS.RepoStats) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetRepoStats(repoStats)
  }

  function onGetNodeInfo (nodeInfo: IPFS.NodeInfo) {
    const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
    actions.onGetNodeInfo(nodeInfo)
  }

  return {
    initialize,
    onGetConnectedPeers,
    onGetAddressesConfig,
    onGetDaemonStatus,
    onGetRepoStats,
    onGetNodeInfo
  }
})

document.addEventListener('DOMContentLoaded', window.ipfs.initialize)
