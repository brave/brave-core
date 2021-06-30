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
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'

// Utils
import store from './store'
import * as ipfsActions from './actions/ipfs_actions'

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
  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <App />
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

function onGetConnectedPeers (peerCount: number) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGetConnectedPeers(peerCount)
}

function onGetAddressesConfig (addressesConfig: IPFS.AddressesConfig) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGetAddressesConfig(addressesConfig)
}

function onInstallationProgress (installationProgress: IPFS.InstallationProgress) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onInstallationProgress(installationProgress)
}

function onGetDaemonStatus (daemonStatus: IPFS.DaemonStatus) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGetDaemonStatus(daemonStatus)
  if (daemonStatus.launched) {
    getConnectedPeers()
    getAddressesConfig()
    getRepoStats()
    getNodeInfo()
  }
}

function onGetRepoStats (repoStats: IPFS.RepoStats) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGetRepoStats(repoStats)
}

function onGetNodeInfo (nodeInfo: IPFS.NodeInfo) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGetNodeInfo(nodeInfo)
}

function onGarbageCollection (garbageCollectionStatus: IPFS.GarbageCollectionStatus) {
  const actions = bindActionCreators(ipfsActions, store.dispatch.bind(store))
  actions.onGarbageCollection(garbageCollectionStatus)
}

// Expose functions to Page Handlers.
// TODO(petemill): Use event listeners instead.
// @ts-ignore
window.ipfs = {
  onGetConnectedPeers,
  onGetAddressesConfig,
  onGetDaemonStatus,
  onGetRepoStats,
  onGetNodeInfo,
  onGarbageCollection,
  onInstallationProgress
}

document.addEventListener('DOMContentLoaded', initialize)
