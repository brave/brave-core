/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_webtorrent/extension/constants/webtorrent_types'
import * as actions from '../../../brave_webtorrent/extension/actions/webtorrent_actions'
import { Torrent } from 'webtorrent'

const torrent: Torrent = {
  infoHash: 'infoHash',
  magnetURI: 'uri',
  timeRemaining: 0,
  downloaded: 0,
  uploaded: 0,
  downloadSpeed: 0,
  uploadSpeed: 0,
  progress: 0,
  ratio: 0,
  numPeers: 0,
  torrentFile: Buffer.from('1337', 'utf-8'),
  torrentFileBlobURL: 'https://brave.com',
  files: [],
  received: 1337,
  path: '/',
  ready: true,
  name: 'torrentz',
  destroy: jest.fn(),
  addPeer: jest.fn(),
  addWebSeed: jest.fn(),
  removePeer: jest.fn(),
  select: jest.fn(),
  deselect: jest.fn(),
  createServer: jest.fn(),
  pause: jest.fn(),
  resume: jest.fn(),
  on: jest.fn(),
  addListener: jest.fn(),
  once: jest.fn(),
  removeListener: jest.fn(),
  off: jest.fn(),
  removeAllListeners: jest.fn(),
  setMaxListeners: jest.fn(),
  getMaxListeners: jest.fn(),
  listeners: jest.fn(),
  rawListeners: jest.fn(),
  emit: jest.fn(),
  listenerCount: jest.fn(),
  prependListener: jest.fn(),
  prependOnceListener: jest.fn(),
  eventNames: jest.fn()
}

describe('webtorrent_actions', () => {
  it('progressUpdated', () => {
    expect(actions.progressUpdated(torrent)).toEqual({
      type: types.WEBTORRENT_PROGRESS_UPDATED,
      meta: undefined,
      payload: { torrent }
    })
  })

  it('infoUpdated', () => {
    expect(actions.infoUpdated(torrent)).toEqual({
      type: types.WEBTORRENT_INFO_UPDATED,
      meta: undefined,
      payload: { torrent }
    })
  })

  it('serverUpdated', () => {
    const serverURL = 'https://localhost:12345'
    expect(actions.serverUpdated(torrent, serverURL)).toEqual({
      type: types.WEBTORRENT_SERVER_UPDATED,
      meta: undefined,
      payload: { torrent, serverURL }
    })
  })

  it('startTorrent', () => {
    const torrentId = 'id'
    const tabId = 1
    expect(actions.startTorrent(torrentId, tabId)).toEqual({
      type: types.WEBTORRENT_START_TORRENT,
      meta: undefined,
      payload: { torrentId, tabId }
    })
  })

  it('stopDownload', () => {
    const tabId = 1
    expect(actions.stopDownload(tabId)).toEqual({
      type: types.WEBTORRENT_STOP_DOWNLOAD,
      meta: undefined,
      payload: { tabId }
    })
  })
})
