/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { debounce } from '../common/debounce'

const keyName = 'ipfs-data'

export const defaultState: IPFS.State = {
  connectedPeers: {
    peerCount: 0
  },
  addressesConfig: {
    api: '',
    gateway: '',
    swarm: []
  },
  daemonStatus: {
    installed: false,
    launched: false,
    restarting: false,
    installing: false,
    error: ''
  },
  garbageCollectionStatus: {
    success: true,
    error: '',
    started: false
  },
  repoStats: {
    objects: 0,
    size: 0,
    storage: 0,
    path: '',
    version: ''
  },
  nodeInfo: {
    id: '',
    version: '',
    component_version: ''
  },
  installationProgress: {
    total_bytes: -1,
    downloaded_bytes: -1
  }
}

export const getLoadTimeData = (state: IPFS.State): IPFS.State => {
  state = { ...state }

  Object.keys(defaultState).forEach(function (key) {
    state[key] = Object.assign({}, defaultState[key], state[key])
  })

  return state
}

export const cleanData = (state: IPFS.State): IPFS.State => {
  return getLoadTimeData(state)
}

export const load = (): IPFS.State => {
  const data = window.localStorage.getItem(keyName)
  let state: IPFS.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: IPFS.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
