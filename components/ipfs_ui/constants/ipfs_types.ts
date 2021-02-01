/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  IPFS_GET_CONNECTED_PEERS = '@@ipfs/IPFS_GET_CONNECTED_PEERS',
  IPFS_ON_GET_CONNECTED_PEERS = '@@ipfs/IPFS_ON_GET_CONNECTED_PEERS',
  IPFS_GET_ADDRESSES_CONFIG = '@@ipfs/IPFS_GET_ADDRESSES_CONFIG',
  IPFS_ON_GET_ADDRESSES_CONFIG = '@@ipfs/IPFS_ON_GET_ADDRESSES_CONFIG',
  IPFS_GET_DAEMON_STATUS = '@@ipfs/IPFS_GET_DAEMON_STATUS',
  IPFS_ON_GET_DAEMON_STATUS = '@@ipfs/IPFS_ON_GET_DAEMON_STATUS',
  IPFS_GET_REPO_STATS = '@@ipfs/IPFS_GET_REPO_STATS',
  IPFS_ON_GET_REPO_STATS = '@@ipfs/IPFS_ON_GET_REPO_STATS',
  IPFS_GET_NODE_INFO = '@@ipfs/IPFS_GET_NODE_INFO',
  IPFS_ON_GET_NODE_INFO = '@@ipfs/IPFS_ON_GET_NODE_INFO',
  IPFS_LAUNCH_DAEMON = '@@ipfs/IPFS_LAUNCH_DAEMON',
  IPFS_SHUTDOWN_DAEMON = '@@ipfs/IPFS_SHUTDOWN_DAEMON'
}
