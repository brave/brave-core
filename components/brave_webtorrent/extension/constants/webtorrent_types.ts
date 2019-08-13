/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  WEBTORRENT_PROGRESS_UPDATED = '@@webtorrent/WEBTORRENT_PROGRESS_UPDATED',
  WEBTORRENT_INFO_UPDATED = '@@webtorrent/WEBTORRENT_INFO_UPDATED',
  WEBTORRENT_SERVER_UPDATED = '@@webtorrent/WEBTORRENT_SERVER_UPDATED',
  WEBTORRENT_START_TORRENT = '@@webtorrent/WEBTORRENT_START_TORRENT',
  WEBTORRENT_STOP_DOWNLOAD = '@@webtorrent/WEBTORRENT_STOP_DOWNLOAD',
  WEBTORRENT_TORRENT_PARSED = '@@webtorrent/WEBTORRENT_TORRENT_PARSED',
  WEBTORRENT_SAVE_ALL_FILES = '@@webtorrent/WEBTORRENT_SAVE_ALL_FILES'
}
