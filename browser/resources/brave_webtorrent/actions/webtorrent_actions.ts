/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'
import { Torrent } from 'webtorrent'

// Constants
import { types } from '../constants/webtorrent_types'

export const progressUpdated = (torrent: Torrent) => action(types.WEBTORRENT_PROGRESS_UPDATED, { torrent })
export const infoUpdated = (torrent: Torrent) => action(types.WEBTORRENT_INFO_UPDATED, { torrent })
export const serverUpdated = (torrent: Torrent, serverURL: string) =>
  action(types.WEBTORRENT_SERVER_UPDATED, { torrent, serverURL })
export const startTorrent = (torrentId: string, tabId: number) => action(types.WEBTORRENT_START_TORRENT, { torrentId, tabId })
export const stopDownload = (tabId: number) => action(types.WEBTORRENT_STOP_DOWNLOAD, { tabId })
