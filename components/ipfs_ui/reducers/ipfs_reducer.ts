/* This Source Code Form is subject to the terms of the Mozilla Public
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
      console.log('action on get connected peers:', action)
      state = {
        ...state,
        connectedPeers: {
          ...state.connectedPeers,
          peerCount: action.payload.peerCount
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
