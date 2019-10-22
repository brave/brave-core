/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const getTorrentState = (state: TorrentsState, tabId: number) => {
  return state.torrentStateMap[tabId]
}

export const getTorrentObj = (state: TorrentsState, tabId: number) => {
  let torrent

  const torrentState = getTorrentState(state, tabId)
  if (torrentState && torrentState.infoHash) {
    torrent = state.torrentObjMap[torrentState.infoHash]
  }

  return torrent
}

export interface ApplicationState {
  torrentsData: TorrentsState
}

export interface TorrentsState {
  torrentStateMap: TorrentStateMap
  torrentObjMap: TorrentObjMap
}

export interface TorrentStateMap {
  [key: number]: TorrentState // tabId as key
}

export interface TorrentObjMap {
  [key: string]: TorrentObj // infoHash as key
}

export interface File {
  name: string
  length: number
}

export interface TorrentState {
  tabId: number
  torrentId: string
  name?: string | string[]
  infoHash?: string
  errorMsg?: string
  ix?: number | number[]
}

export interface TorrentObj {
  name?: string,
  files?: File[]
  serverURL?: string
  timeRemaining: number
  downloaded: number
  uploaded: number
  downloadSpeed: number
  uploadSpeed: number
  progress: number
  ratio: number
  numPeers: number
  length: number
  tabClients: Set<number>
}
