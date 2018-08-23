/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as WebTorrent from 'webtorrent'
import { addTorrentEvents } from './events/torrentEvents'
import { addWebtorrentEvents } from './events/webtorrentEvents'
import { AddressInfo } from 'net'

let webTorrent: WebTorrent.Instance
let servers: { [key: string]: any } = { }

export const init = () => {
  webTorrent = new WebTorrent({ tracker: { wrtc: false } })
  addWebtorrentEvents(webTorrent)
}

export const getWebTorrent = () => webTorrent

export const createServer = (torrent: WebTorrent.Torrent, cb: (serverURL: string) => void) => {
  if (!torrent.infoHash) return // torrent is not ready

  const server = torrent.createServer()
  if (!server) return

  try {
    server.listen(0, '127.0.0.1', undefined, () => {
      // Explicitly cast server.address() to AddressInfo to access its
      // properties. It's safe to cast here because the only possible type of
      // server.address() here is AddressInfo since pipe name (as string) is
      // not supported in chrome-net.
      const addrInfo = server.address() as AddressInfo
      servers[torrent.infoHash] = server
      cb('http://' + addrInfo.address + ':' + addrInfo.port.toString())
    })
  } catch (error) {
    console.log('server listen error: ', error)
  }
}

export const addTorrent = (torrentId: string) => {
  const torrentObj = webTorrent.add(torrentId)
  addTorrentEvents(torrentObj)
}

export const findTorrent = (infoHash: string) => {
  return webTorrent.torrents.find(torrent => torrent.infoHash === infoHash)
}

export const delTorrent = (infoHash: string) => {
  const torrent = findTorrent(infoHash)
  if (torrent) torrent.destroy()
  if (servers[infoHash]) {
    servers[infoHash].close()
    delete servers[infoHash]
  }
}
