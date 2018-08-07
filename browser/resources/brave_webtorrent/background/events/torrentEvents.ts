/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Torrent } from 'webtorrent'
import * as throttle from 'throttleit'

import webtorrentActions from '../actions/webtorrentActions'
import { createServer } from '../webtorrent'

export const addTorrentEvents = (torrent: Torrent) => {
  torrent.on('done', () => {
    webtorrentActions.progressUpdated(torrent)
  })
  torrent.on('infoHash', () => {
    console.log('infoHash event')
    webtorrentActions.infoUpdated(torrent)
  })
  torrent.on('metadata', () => {
    console.log('metadata event')
    webtorrentActions.infoUpdated(torrent)
  })
  torrent.on('download', throttle((bytes: number) => {
    webtorrentActions.progressUpdated(torrent)
  }, 1000))
  torrent.on('upload', throttle((bytes: number) => {
    webtorrentActions.progressUpdated(torrent)
  }, 1000))
  torrent.on('ready', () => {
    console.log('ready', torrent)
    createServer(torrent, (serverURL: string) => {
      webtorrentActions.serverUpdated(torrent, serverURL)
    })
  })
  torrent.on('warning', (e: Error | string) => {
    console.log('warning: ', torrent, e)
  })
  torrent.on('error', (e: Error | string) => {
    console.log('error: ', torrent, e)
  })
}
