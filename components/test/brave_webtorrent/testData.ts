/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ActiveTabIds, ApplicationState, TorrentObj, TorrentState, TorrentStateMap, TorrentObjMap, TorrentsState }  from '../../brave_webtorrent/extension/constants/webtorrentState.ts'

export const currentWindowId : number = 0

export const activeTabIds : ActiveTabIds = { 0: 0, 1: 1 }

export const torrentState : TorrentState = {
  tabId: 0, torrentId: 'testTorrentId'
}

export const torrentObj : TorrentObj = {
  timeRemaining: 0,
  downloaded: 0,
  uploaded: 0,
  downloadSpeed: 0,
  uploadSpeed: 0,
  progress: 0,
  ratio: 0,
  numPeers: 0,
  length: 0,
  tabClients: new Set<number>()
}

export const torrentStateMap : TorrentStateMap = {
  0: torrentState
}

export const torrentObjMap : TorrentObjMap = {
  'infoHash': torrentObj
}

export const torrentsState: TorrentsState = {
  activeTabIds,
  currentWindowId,
  torrentStateMap,
  torrentObjMap
}

export const applicationState: ApplicationState = {
  torrentsData: torrentsState
}
