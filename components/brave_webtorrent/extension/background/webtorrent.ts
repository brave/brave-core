/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as WebTorrent from 'webtorrent'
import { addTorrentEvents, removeTorrentEvents } from './events/torrentEvents'
import { addWebtorrentEvents } from './events/webtorrentEvents'
import { AddressInfo } from 'net'
import { Instance } from 'parse-torrent'
import { basename, extname } from 'path'
import * as JSZip from 'jszip'

let webTorrent: WebTorrent.Instance | undefined
let servers: { [key: string]: any } = { }

export const getWebTorrent = () => {
  if (!webTorrent) {
    webTorrent = new WebTorrent({ tracker: { wrtc: false } })
    addWebtorrentEvents(webTorrent)
  }

  return webTorrent
}

export const createServer = (torrent: WebTorrent.Torrent, cb: (serverURL: string) => void) => {
  if (!torrent.infoHash) return // torrent is not ready

  const opts = {
    // Only allow requests from this origin ('chrome-extension://...) so
    // websites cannot violate same-origin policy by reading contents of
    // active torrents.
    origin: window.location.origin,
    // Use hostname option to mitigate DNS rebinding
    // Ref: https://github.com/brave/browser-laptop/issues/12616
    hostname: '127.0.0.1'
  }
  const server = torrent.createServer(opts)
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

export const addTorrent = (torrentId: string | Instance) => {
  const torrent = getWebTorrent().add(torrentId)
  addTorrentEvents(torrent)
}

export const findTorrent = (infoHash: string) => {
  return getWebTorrent().torrents.find(torrent => torrent.infoHash === infoHash)
}

const maybeDestroyWebTorrent = () => {
  if (!webTorrent || webTorrent.torrents.length !== 0) return
  webTorrent.destroy()
  webTorrent = undefined
}

export const delTorrent = (infoHash: string) => {
  const torrent = findTorrent(infoHash)
  if (torrent) {
    removeTorrentEvents(torrent)
    torrent.destroy()
  }

  if (servers[infoHash]) {
    servers[infoHash].close()
    delete servers[infoHash]
  }

  maybeDestroyWebTorrent()
}

export const saveAllFiles = (infoHash: string) => {
  const torrent = findTorrent(infoHash)

  if (!torrent || !torrent.name || !torrent.files) return

  const files: WebTorrent.TorrentFile[] = torrent.files

  let zip: any = new JSZip()
  const zipFilename = basename(torrent.name, extname(torrent.name)) + '.zip'

  const downloadBlob = (blob: Blob) => {
    const url = URL.createObjectURL(blob)

    chrome.downloads.download({
      url: url,
      filename: zipFilename,
      saveAs: false,
      conflictAction: 'uniquify'
    })
  }

  const downloadZip = () => {
    if (files.length > 1) {
      // generate the zip relative to the torrent folder
      zip = zip.folder(torrent.name)
    }

    zip.generateAsync({ type: 'blob' })
      .then(
        (blob: Blob) => downloadBlob(blob),
        (err: Error) => console.error(err)
      )
  }

  const addFilesToZip = () => {
    let addedFiles = 0

    files.forEach(file => {
      file.getBlob((err, blob) => {
        if (err) {
          console.error(err)
        } else {
          // add file to zip
          zip.file(file.path, blob)
        }

        addedFiles += 1

        // start the download when all files have been added
        if (addedFiles === files.length) {
          downloadZip()
        }
      })
    })
  }

  addFilesToZip()
}
