/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as ParseTorrent from 'parse-torrent'
import { Torrent } from 'webtorrent'
import { parse } from 'querystring'

// Constants
import * as tabTypes from '../../constants/tab_types'
import * as torrentTypes from '../../constants/webtorrent_types'
import { File, TorrentState, TorrentsState } from '../../constants/webtorrentState'

// Utils
import {
  addTorrent,
  delTorrent,
  findTorrent,
  saveAllFiles as webtorrentSaveAllFiles
} from '../webtorrent'
import { parseTorrentRemote } from '../api/torrent_api'

const tabUpdated = (tabId: number, url: string, state: TorrentsState) => {
  const { torrentStateMap, torrentObjMap } = state
  const origTorrentState: TorrentState = torrentStateMap[tabId]
  const origInfoHash = origTorrentState ? origTorrentState.infoHash : undefined
  let newTorrentState: TorrentState | undefined
  let newInfoHash: string | undefined

  // create new torrent state
  const parsedURL = new window.URL(url)
  const torrentId = parsedURL.href
  if (parsedURL.protocol === 'magnet:') { // parse torrent
    try {
      const { name, infoHash, ix } = ParseTorrent(torrentId)
      newInfoHash = infoHash
      newTorrentState = { tabId, torrentId, name, infoHash, ix }
    } catch (error) {
      newTorrentState = { tabId, torrentId, errorMsg: error.message }
    }
  } else if (parsedURL.protocol === 'https:' || parsedURL.protocol === 'http:') {
    const name = parsedURL.pathname.substr(parsedURL.pathname.lastIndexOf('/') + 1)
    // for .torrent case, ix (index of file) for selecting a specific file in
    // the file list is given in url like #ix=5
    let ix: number | undefined = Number(parse(parsedURL.hash.slice(1)).ix)
    ix = Number.isNaN(ix) ? undefined : ix

    // Use an existing infoHash if it's the same torrentId
    const torrentUrl = parsedURL.origin + parsedURL.pathname
    const key = Object.keys(torrentStateMap).find(
      key => torrentStateMap[key].infoHash &&
        torrentStateMap[key].torrentId === torrentUrl)
    newInfoHash = key
      ? torrentStateMap[key].infoHash
      : undefined

    newTorrentState = { tabId, torrentId, name, ix, infoHash: newInfoHash }
  }

  // delete old torrent state
  delete torrentStateMap[tabId]

  // unsubscribe old torrent if not the same
  const isSameTorrent = newInfoHash && origInfoHash === newInfoHash
  if (origInfoHash && torrentObjMap[origInfoHash] && !isSameTorrent) {
    torrentObjMap[origInfoHash].tabClients.delete(tabId)
    if (!torrentObjMap[origInfoHash].tabClients.size) {
      delete torrentObjMap[origInfoHash]
      delTorrent(origInfoHash)
    }
  }

  // save new torrent state and subscribe directly if torrent already existed
  if (newTorrentState) {
    torrentStateMap[tabId] = newTorrentState
    if (newInfoHash && torrentObjMap[newInfoHash]) {
      torrentObjMap[newInfoHash].tabClients.add(tabId)
    }
  }

  return { ...state, torrentStateMap, torrentObjMap }
}

const tabRemoved = (tabId: number, state: TorrentsState) => {
  const { torrentStateMap, torrentObjMap } = state
  const torrentState = torrentStateMap[tabId]
  if (torrentState) {
    const infoHash = torrentState.infoHash
    if (infoHash && torrentObjMap[infoHash]) { // unsubscribe
      if (torrentObjMap[infoHash].tabClients) {
        torrentObjMap[infoHash].tabClients.delete(tabId)
      }
      if (!torrentObjMap[infoHash].tabClients.size) {
        delete torrentObjMap[infoHash]
        delTorrent(infoHash)
      }
    }
    delete torrentStateMap[tabId]
  }

  return { ...state, torrentStateMap, torrentObjMap }
}

const startTorrent = (torrentId: string, tabId: number, state: TorrentsState) => {
  const { torrentStateMap, torrentObjMap } = state
  const torrentState = torrentStateMap[tabId]

  // infoHash might not be available yet for .torrent case
  if (torrentState && !torrentState.errorMsg && !torrentState.infoHash) {
    parseTorrentRemote(torrentId, tabId)
  }

  if (torrentState && torrentState.infoHash &&
    !findTorrent(torrentState.infoHash)) {
    addTorrent(torrentId) // objectMap will be updated when info event is emitted
  } else if (torrentState && torrentState.infoHash &&
    torrentObjMap[torrentState.infoHash]) {
    torrentObjMap[torrentState.infoHash].tabClients.add(tabId)
  }

  return { ...state, torrentObjMap }
}

const stopDownload = (tabId: number, state: TorrentsState) => {
  const { torrentStateMap, torrentObjMap } = state
  const infoHash = torrentStateMap[tabId].infoHash

  if (infoHash && torrentObjMap[infoHash]) {
    delete torrentObjMap[infoHash]
    delTorrent(infoHash)
  }

  return { ...state, torrentStateMap, torrentObjMap }
}

const updateProgress = (state: TorrentsState, torrent: Torrent) => {
  const { torrentObjMap } = state
  // don't add a new entry since the download might be stopped already
  if (!torrentObjMap[torrent.infoHash]) {
    return state
  }

  const { downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining } = torrent
  torrentObjMap[torrent.infoHash] = { ...torrentObjMap[torrent.infoHash],
    downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining }

  return { ...state, torrentObjMap }
}

const updateInfo = (state: TorrentsState, torrent: Torrent) => {
  const { torrentStateMap, torrentObjMap } = state
  const { name, downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining, infoHash } = torrent
  let length: number = 0
  const files: File[] = torrent.files.map((file) => {
    length += file.length
    return { name: file.name, length: file.length }
  })

  const tabClients: Set<number> = new Set<number>()
  Object.keys(torrentStateMap).filter(
    key => torrentStateMap[key].infoHash === infoHash).map(
      key => {
        tabClients.add(torrentStateMap[key].tabId)
      }
    )

  torrentObjMap[torrent.infoHash] = { ...torrentObjMap[torrent.infoHash],
    files, name, downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining, length, tabClients }

  return { ...state, torrentStateMap, torrentObjMap }
}

const updateServer = (state: TorrentsState, torrent: Torrent, serverURL: string) => {
  const { torrentObjMap } = state
  torrentObjMap[torrent.infoHash] = { ...torrentObjMap[torrent.infoHash], serverURL }
  return { ...state, torrentObjMap }
}

const torrentParsed = (torrentId: string, tabId: number, infoHash: string | undefined, errorMsg: string | undefined, parsedTorrent: ParseTorrent.Instance | undefined, state: TorrentsState) => {
  const { torrentObjMap, torrentStateMap } = state
  torrentStateMap[tabId] = { ...torrentStateMap[tabId], infoHash, errorMsg }

  if (infoHash && !findTorrent(infoHash) && parsedTorrent) {
    addTorrent(parsedTorrent) // objectMap will be updated when info event is emitted
  } else if (infoHash && torrentObjMap[infoHash]) {
    torrentObjMap[infoHash].tabClients.add(tabId)
  }

  return { ...state, torrentObjMap, torrentStateMap }
}

const saveAllFiles = (state: TorrentsState, infoHash: string) => {
  webtorrentSaveAllFiles(infoHash)
  return { ...state }
}

const defaultState: TorrentsState = { torrentStateMap: {}, torrentObjMap: {} }
export const webtorrentReducer = (state: TorrentsState = defaultState, action: any) => { // TODO: modify any to be actual action type
  const payload = action.payload
  switch (action.type) {
    case tabTypes.types.TAB_UPDATED:
      if (payload.changeInfo.url) {
        state = tabUpdated(payload.changeInfo.tabId, payload.changeInfo.url, state)
      }
      break
    case tabTypes.types.TAB_REMOVED:
      state = tabRemoved(payload.tabId, state)
      break
    case tabTypes.types.TAB_RETRIEVED:
      if (payload.tab.id && payload.tab.url) {
        state = tabUpdated(payload.tab.id, payload.tab.url, state)
      }
      break
    case torrentTypes.types.WEBTORRENT_PROGRESS_UPDATED:
      state = updateProgress(state, payload.torrent)
      break
    case torrentTypes.types.WEBTORRENT_INFO_UPDATED:
      state = updateInfo(state, payload.torrent)
      break
    case torrentTypes.types.WEBTORRENT_SERVER_UPDATED:
      state = updateServer(state, payload.torrent, payload.serverURL)
      break
    case torrentTypes.types.WEBTORRENT_START_TORRENT:
      state = startTorrent(payload.torrentId, payload.tabId, state)
      break
    case torrentTypes.types.WEBTORRENT_STOP_DOWNLOAD:
      state = stopDownload(payload.tabId, state)
      break
    case torrentTypes.types.WEBTORRENT_TORRENT_PARSED:
      state = torrentParsed(payload.torrentId, payload.tabId, payload.infoHash,
        payload.errorMsg, payload.parsedTorrent, state)
      break
    case torrentTypes.types.WEBTORRENT_SAVE_ALL_FILES:
      state = saveAllFiles(state, payload.infoHash)
      break
  }

  // console.error("Got state", JSON.stringify(state));
  return state
}
