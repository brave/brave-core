/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as ParseTorrent from 'parse-torrent'
import { Torrent } from 'webtorrent'

// Constants
import * as tabTypes from '../../constants/tab_types'
import * as windowTypes from '../../constants/window_types'
import * as torrentTypes from '../../constants/webtorrent_types'
import { File, TorrentState, TorrentsState } from '../../constants/webtorrentState'

// Utils
import { addTorrent, delTorrent, findTorrent } from '../webtorrent'

const isTorrentPage = (url: URL) => {
  const fullpath = url.origin + url.pathname
  return fullpath === chrome.runtime.getURL('brave_webtorrent.html')
}

const focusedWindowChanged = (windowId: number, state: TorrentsState) => {
  return { ...state, currentWindowId: windowId}
}

const windowRemoved = (windowId: number, state: TorrentsState) => {
  const { activeTabIds } = state
  delete activeTabIds[windowId]
  return { ...state, activeTabIds }
}

const activeTabChanged = (tabId: number, windowId: number, state: TorrentsState) => {
  const { activeTabIds } = state
  activeTabIds[windowId] = tabId
  return { ...state, activeTabIds }
}

const tabUpdated = (tabId: number, url: string, state: TorrentsState) => {
  const { torrentStateMap, torrentObjMap } = state
  const origTorrentState: TorrentState = torrentStateMap[tabId]
  const origInfoHash =  origTorrentState ? origTorrentState.infoHash : undefined
  let newTorrentState, newInfoHash

  // delete old torrent state
  delete torrentStateMap[tabId] // delete old torrent state

  // create new torrent state
  const parsedURL = new window.URL(url)
  if (isTorrentPage(parsedURL)) { // parse torrent
    const torrentId = decodeURIComponent(parsedURL.search.substring(1))
    try {
      const { name, infoHash, ix } = ParseTorrent(torrentId)
      newInfoHash = infoHash
      newTorrentState = { tabId, torrentId, name, infoHash, ix }
    } catch (error) {
      newTorrentState = { tabId, torrentId, errorMsg: error.message }
    }
  }

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
      torrentObjMap[infoHash].tabClients.delete(tabId)
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

  if (torrentState && torrentState.infoHash &&
    !findTorrent(torrentState.infoHash)) {
    addTorrent(torrentId) // objectMap will be updated when info event is emitted
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
  const { torrentObjMap  } = state
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
  const { downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining, infoHash } = torrent
  let length = 0
  const files : File[] = torrent.files.map((file) => {
    length += file.length
    return { name: file.name, length: file.length }
  })

  const tabClients : Set<number> = new Set<number>()
  Object.keys(torrentStateMap).filter(
    key => torrentStateMap[key].infoHash === infoHash).map(
      key => {
        tabClients.add(torrentStateMap[key].tabId)
      }
    )

  torrentObjMap[torrent.infoHash] = { ...torrentObjMap[torrent.infoHash],
    files, downloaded, uploaded, downloadSpeed, uploadSpeed, progress, ratio,
    numPeers, timeRemaining, length, tabClients }

  return { ...state, torrentStateMap, torrentObjMap }
}

const updateServer = (state: TorrentsState, torrent: Torrent, serverURL: string) => {
  const { torrentObjMap } = state
  torrentObjMap[torrent.infoHash] = { ...torrentObjMap[torrent.infoHash], serverURL }
  return { ...state, torrentObjMap }
}

const defaultState : TorrentsState = { currentWindowId: -1, activeTabIds: {}, torrentStateMap: {}, torrentObjMap: {} }
const webtorrentReducer = (state: TorrentsState = defaultState, action: any) => { // TODO: modify any to be actual action type
  const payload = action.payload
  switch (action.type) {
    case windowTypes.types.WINDOW_CREATED:
      if (payload.window.focused || Object.keys(state.activeTabIds).length === 0) {
        state = focusedWindowChanged(payload.window.id, state)
      }
      break
    case windowTypes.types.WINDOW_REMOVED:
      state = windowRemoved(payload.windowId, state)
      break
    case windowTypes.types.WINDOW_FOCUS_CHANGED:
      state = focusedWindowChanged(payload.windowId, state)
      break
    case tabTypes.types.ACTIVE_TAB_CHANGED:
      state = activeTabChanged(payload.tabId, payload.windowId, state)
      break
    case tabTypes.types.TAB_CREATED:
      if (payload.tab.id && payload.tab.url) {
        state = tabUpdated(payload.tab, payload.tab.url, state)
      }
      break
    case tabTypes.types.TAB_UPDATED:
      if (payload.changeInfo.url) {
        state = tabUpdated(payload.tab.id, payload.changeInfo.url, state)
      }
      break
    case tabTypes.types.TAB_REMOVED:
      state = tabRemoved(payload.tabId, state)
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
  }

  return state
}

export default webtorrentReducer
