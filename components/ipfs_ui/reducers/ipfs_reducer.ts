/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/ipfs_types'

// Utils
import * as storage from '../storage'

const kNodeWebUIUrl = 'http://127.0.0.1:{api-port}/webui'
const kPeersWebUIUrl = 'http://127.0.0.1:{api-port}/webui/#/peers'

// api param is expected in format like /ip4/127.0.0.1/tcp/45001
// where 45001 will be used as {api-port} value for target
const openURlInNewTab = (target: string, api: string) => {
  if (!api.length) {
    return
  }
  const port = api.slice(api.lastIndexOf('/') + 1, api.length)
  window.open(target.replace('{api-port}', port), '_blank')
}

const ipfsReducer: Reducer<IPFS.State | undefined> = (state: IPFS.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.IPFS_GET_CONNECTED_PEERS:
      chrome.send('ipfs.getConnectedPeers')
      break
    case types.IPFS_ON_GET_CONNECTED_PEERS:
      state = {
        ...state,
        connectedPeers: action.payload.connectedPeers
      }
      break
    case types.IPFS_GET_NODE_INFO:
      chrome.send('ipfs.getNodeInfo')
      break
    case types.IPFS_ON_GET_NODE_INFO:
      state = {
        ...state,
        nodeInfo: action.payload.nodeInfo
      }
      break
    case types.IPFS_GET_REPO_STATS:
      chrome.send('ipfs.getRepoStats')
      break
    case types.IPFS_ON_GET_REPO_STATS:
      state = {
        ...state,
        repoStats: action.payload.repoStats
      }
      break
    case types.IPFS_GET_ADDRESSES_CONFIG:
      chrome.send('ipfs.getAddressesConfig')
      break
    case types.IPFS_ON_GET_ADDRESSES_CONFIG:
      state = {
        ...state,
        addressesConfig: action.payload.addressesConfig
      }
      break
    case types.IPFS_GET_DAEMON_STATUS:
      chrome.send('ipfs.getDaemonStatus')
      break
    case types.IPFS_ON_GET_DAEMON_STATUS:
      state = {
        ...state,
        daemonStatus: action.payload.daemonStatus
      }
      if (action.payload.daemonStatus.launched) {
        state.garbageCollectionStatus.error = ''
        state.garbageCollectionStatus.success = true
        state.garbageCollectionStatus.started = false
      }
      break
    case types.IPFS_INSTALL_DAEMON:
      chrome.send('ipfs.launchDaemon')
      state = {
        ...state,
        daemonStatus: {
          installed: false,
          launched: false,
          restarting: false,
          installing: true,
          error: ''
        }
      }
      break
    case types.IPFS_LAUNCH_DAEMON:
      chrome.send('ipfs.launchDaemon')
      break
    case types.IPFS_SHUTDOWN_DAEMON:
      chrome.send('ipfs.shutdownDaemon')
      break
    case types.IPFS_OPEN_NODE_WEBUI:
      openURlInNewTab(kNodeWebUIUrl, state.addressesConfig.api)
      break
    case types.IPFS_OPEN_PEERS_WEBUI:
      openURlInNewTab(kPeersWebUIUrl, state.addressesConfig.api)
      break
    case types.IPFS_GARBAGE_COLLECTION:
      chrome.send('ipfs.garbageCollection')
      state = {
        ...state,
        garbageCollectionStatus: {
          error: '',
          success: true,
          started: true
        }
      }
      break
    case types.IPFS_ON_GARBAGE_COLLECTION:
      state = {
        ...state,
        garbageCollectionStatus: action.payload.garbageCollectionStatus
      }
      break
    case types.IPFS_ON_INSTALLATION_PROGRESS:
      state = {
        ...state,
        installationProgress: action.payload.installationProgress
      }
      break
    case types.IPFS_RESTART_DAEMON:
      chrome.send('ipfs.restartDaemon')
      state = {
        ...state,
        daemonStatus: {
          installed: true,
          launched: false,
          restarting: true,
          installing: false,
          error: ''
        }
      }
      break
    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default ipfsReducer
