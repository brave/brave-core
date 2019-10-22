/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { webtorrentReducer } from '../../../../brave_webtorrent/extension/background/reducers/webtorrent_reducer'
import * as tabActions from '../../../../brave_webtorrent/extension/actions/tab_actions'
import { torrentObjMap } from '../../testData'

// this import seems to trigger createStore and get an undefined reducer
jest.mock('../../../../brave_webtorrent/extension/background/events/torrentEvents')

const defaultState = { torrentStateMap: {}, torrentObjMap: {} }
describe('webtorrent reducer test', () => {
  it('handle the initial state', () => {
    const state = webtorrentReducer(undefined, tabActions.tabRemoved(1))
    expect(state).toEqual(defaultState)
  })

  const tab: chrome.tabs.Tab = {
    discarded: false,
    autoDiscardable: false,
    id: 0,
    index: 2,
    pinned: false,
    highlighted: false,
    windowId: 0,
    active: false,
    incognito: false,
    selected: false
  }

  describe('TAB_UPDATED', () => {
    // TODO: mock ParseTorrent to test tab url case
  })

  describe('TAB_REMOVED', () => {
    it('removed tab state is removed', () => {
      const setupTorrentsState: TorrentsState = {
        torrentStateMap: {
          0: {
            tabId: 0, torrentId: 'testTorrentId', infoHash: 'infoHash'
          }
        },
        torrentObjMap
      }

      const state = webtorrentReducer(setupTorrentsState,
        tabActions.tabRemoved(tab.id))
      expect(state).toEqual(defaultState)
    })
  })

})
