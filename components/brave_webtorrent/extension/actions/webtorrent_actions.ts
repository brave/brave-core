/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'
import { Torrent } from 'webtorrent'
import { Instance } from 'parse-torrent'

// Constants
import { types } from '../constants/webtorrent_types'

export const progressUpdated = (torrent: Torrent) => action(types.WEBTORRENT_PROGRESS_UPDATED, { torrent })
export const infoUpdated = (torrent: Torrent) => action(types.WEBTORRENT_INFO_UPDATED, { torrent })
export const serverUpdated = (torrent: Torrent, serverURL: string) =>
  action(types.WEBTORRENT_SERVER_UPDATED, { torrent, serverURL })
export const startTorrent = (torrentId: string, tabId: number) => action(types.WEBTORRENT_START_TORRENT, { torrentId, tabId })
export const stopDownload = (tabId: number) => action(types.WEBTORRENT_STOP_DOWNLOAD, { tabId })
export const torrentParsed = (torrentId: string, tabId: number, infoHash: string | undefined, errorMsg: string | undefined, parsedTorrent?: Instance) => action(types.WEBTORRENT_TORRENT_PARSED, { torrentId, tabId, infoHash, errorMsg, parsedTorrent })
export const saveAllFiles = (infoHash: string) => action(types.WEBTORRENT_SAVE_ALL_FILES, { infoHash })
