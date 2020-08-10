/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/ipfs_types'

export const getConnectedPeers = () => action(types.IPFS_GET_CONNECTED_PEERS)

export const onGetConnectedPeers = (peerCount: number) =>
  action(types.IPFS_ON_GET_CONNECTED_PEERS, {
    peerCount
  })
