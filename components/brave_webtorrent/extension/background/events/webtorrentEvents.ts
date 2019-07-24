/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Instance } from 'webtorrent'
import webtorrentActions from '../actions/webtorrentActions'

let interval: number

export const addWebtorrentEvents = (webtorrent: Instance) => {
  webtorrent.on('error', (e: Error | string) => {
    console.log('WebTorrent error: ', e)
  })

  // Always update stats at least once every 5 seconds, even when no torrent
  // events fire
  interval = setInterval(() => {
    webtorrent.torrents.forEach(torrent => {
      webtorrentActions.progressUpdated(torrent)
    })
  }, 5000)
}

export const removeWebTorrentEvents = (webtorrent: Instance) => {
  clearInterval(interval)
}
