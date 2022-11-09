/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/ipfs_types'

export const getConnectedPeers = () => action(types.IPFS_GET_CONNECTED_PEERS)

export const onGetConnectedPeers = (connectedPeers: IPFS.ConnectedPeers) =>
  action(types.IPFS_ON_GET_CONNECTED_PEERS, {
    connectedPeers
  })

export const getAddressesConfig = () => action(types.IPFS_GET_ADDRESSES_CONFIG)

export const onGetAddressesConfig = (addressesConfig: IPFS.AddressesConfig) =>
  action(types.IPFS_ON_GET_ADDRESSES_CONFIG, {
    addressesConfig
  })

export const getDaemonStatus = () => action(types.IPFS_GET_DAEMON_STATUS)

export const onGetDaemonStatus = (daemonStatus: IPFS.DaemonStatus) =>
  action(types.IPFS_ON_GET_DAEMON_STATUS, {
    daemonStatus
  })

export const onInstallationProgress = (installationProgress: IPFS.InstallationProgress) =>
  action(types.IPFS_ON_INSTALLATION_PROGRESS, {
    installationProgress
  })

export const getRepoStats = () => action(types.IPFS_GET_REPO_STATS)

export const onGetRepoStats = (repoStats: IPFS.RepoStats) =>
  action(types.IPFS_ON_GET_REPO_STATS, {
    repoStats
  })

export const getNodeInfo = () => action(types.IPFS_GET_NODE_INFO)

export const onGetNodeInfo = (nodeInfo: IPFS.NodeInfo) =>
  action(types.IPFS_ON_GET_NODE_INFO, {
    nodeInfo
  })

export const launchDaemon = () => action(types.IPFS_LAUNCH_DAEMON)
export const shutdownDaemon = () => action(types.IPFS_SHUTDOWN_DAEMON)
export const restartDaemon = () => action(types.IPFS_RESTART_DAEMON)
export const installDaemon = () => action(types.IPFS_INSTALL_DAEMON)
export const openNodeWebUI = () => action(types.IPFS_OPEN_NODE_WEBUI)
export const openPeersWebUI = () => action(types.IPFS_OPEN_PEERS_WEBUI)

export const garbageCollection = () => action(types.IPFS_GARBAGE_COLLECTION)

export const onGarbageCollection = (garbageCollectionStatus: IPFS.GarbageCollectionStatus) =>
  action(types.IPFS_ON_GARBAGE_COLLECTION, {
    garbageCollectionStatus
  })
