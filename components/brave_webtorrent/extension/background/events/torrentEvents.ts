/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Torrent } from 'webtorrent'
import * as throttle from 'throttleit'

import webtorrentActions from '../actions/webtorrentActions'
import { createServer } from '../webtorrent'

interface TorrentExtended extends Torrent {
  progressInterval?: number
}

export const addTorrentEvents = (torrent: TorrentExtended) => {
  torrent.on('warning', (e: Error) => console.log('warning: ', torrent, e))
  torrent.on('error', (e: Error) => console.log('error: ', torrent, e))

  torrent.on('ready', () => {
    createServer(torrent, (serverURL: string) => {
      webtorrentActions.serverUpdated(torrent, serverURL)
    })
  })

  torrent.on('infoHash', () => webtorrentActions.infoUpdated(torrent))
  torrent.on('metadata', () => webtorrentActions.infoUpdated(torrent))

  const progressUpdatedThrottled = throttle((bytes: number) => {
    webtorrentActions.progressUpdated(torrent)
  }, 500)

  torrent.on('download', progressUpdatedThrottled)
  torrent.on('upload', progressUpdatedThrottled)
  torrent.on('done', progressUpdatedThrottled)
  torrent.on('wire', progressUpdatedThrottled)

  // Always update stats at least once every 5 seconds, even when no torrent
  // events fire. Ensures that download/upload speeds do not get stuck on a
  // positive value when the transfer stalls.
  torrent.progressInterval = window.setInterval(progressUpdatedThrottled, 5000)
}

export const removeTorrentEvents = (torrent: TorrentExtended) => {
  window.clearInterval(torrent.progressInterval)
}
