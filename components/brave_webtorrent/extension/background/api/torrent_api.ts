/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as ParseTorrent from 'parse-torrent'

export const parseTorrentRemote = (torrentId: string, tabId: number) => {
  ParseTorrent.remote(torrentId, (err: Error, parsedTorrent?: ParseTorrent.Instance) => {
    const webtorrentActions = require('../actions/webtorrentActions').default
    const errMsg = err ? err.message : undefined
    const infoHash = parsedTorrent ? parsedTorrent.infoHash : undefined
    webtorrentActions.torrentParsed(torrentId, tabId, infoHash, errMsg, parsedTorrent)
  })
}
