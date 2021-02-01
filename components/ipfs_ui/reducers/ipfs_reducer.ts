/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/ipfs_types'

// Utils
import * as storage from '../storage'

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
        connectedPeers: {
          ...state.connectedPeers,
          peerCount: action.payload.peerCount
        }
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
      break
    case types.IPFS_LAUNCH_DAEMON:
      chrome.send('ipfs.launchDaemon')
      break
    case types.IPFS_SHUTDOWN_DAEMON:
      chrome.send('ipfs.shutdownDaemon')
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
