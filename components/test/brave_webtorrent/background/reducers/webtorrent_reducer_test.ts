/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { webtorrentReducer } from '../../../../brave_webtorrent/extension/background/reducers/webtorrent_reducer'
import * as tabActions from '../../../../brave_webtorrent/extension/actions/tab_actions'
import * as webtorrentActions from '../../../../brave_webtorrent/extension/actions/webtorrent_actions'
import * as windowActions from '../../../../brave_webtorrent/extension/actions/window_actions'
import { torrentsState } from '../../testData'

// this import seems to trigger createStore and get an undefined reducer
jest.mock('../../../../brave_webtorrent/extension/background/events/torrentEvents')

const defaultState = { currentWindowId: -1, activeTabIds: {},
  torrentStateMap: {}, torrentObjMap: {} }
describe('webtorrent reducer test', () => {
  it('handle the initial state', () => {
    const state = webtorrentReducer(undefined, tabActions.tabRemoved({ tabId: 1 }))
    expect(state).toEqual(defaultState)
  })

  describe('WINDOW_CREATED', () => {
    it('sets currentWindowId if it is the first window', () => {
      const window: chrome.windows.Window = {
        id: 1,
        focused: false,
        incognito: false,
        alwaysOnTop: false
      }
      const state = webtorrentReducer(defaultState,
        windowActions.windowCreated(window))
      expect(state).toEqual({ ...defaultState, currentWindowId: 1 })
    })

    it('sets currentWindowId if it is focused', () => {
      const window: chrome.windows.Window = {
        id: 2,
        focused: true,
        incognito: false,
        alwaysOnTop: false
      }
      const state = webtorrentReducer(torrentsState,
        windowActions.windowCreated(window))
      expect(state).toEqual({ ...torrentsState, currentWindowId: 2 })
    })

    it('do not set currentWindowId if either focused or it is the first window', () => {
      const window: chrome.windows.Window = {
        id: 1,
        focused: false,
        incognito: false,
        alwaysOnTop: false
      }
      const state = webtorrentReducer(torrentsState,
        windowActions.windowCreated(window))
      expect(state).toEqual({ ...torrentsState, currentWindowId: 0 })
    })
  })

  describe('WINDOW_REMOVED', () => {
    it('entry from activeTabIds should be deleted when window is removed', () => {
      const state = webtorrentReducer(torrentsState,
        windowActions.windowRemoved(0))
      expect(state.activeTabIds).toEqual({1: 1})
    })
  })

  describe('WINDOW_FOCUS_CHANGED', () => {
    it('currentWindowID should be updated when window focus changed', () => {
      const state = webtorrentReducer(torrentsState,
        windowActions.windowFocusChanged(1))
      expect(state.currentWindowId).toEqual(1)
    })
  })

  const tab: chrome.tabs.Tab = {
    id: 2,
    index: 2,
    pinned: false,
    highlighted: false,
    windowId: 0,
    active: true,
    incognito: false,
    selected: false
  }
  describe('TAB_CREATED', () => {
    it('state is unchanged if tab url is not ready', () => {
      const state = webtorrentReducer(torrentsState,
        tabActions.tabCreated(tab))
      expect(state).toEqual(state)
    })
    // TODO: mock ParseTorrent to test tab url case
  })

  describe('TAB_UPDATED', () => {
    it('state is unchanged if tab url is not ready', () => {
      const changeInfo = {}
      const state = webtorrentReducer(torrentsState,
        tabActions.tabUpdated(tab, changeInfo))
      expect(state).toEqual(state)
    })
    // TODO: mock ParseTorrent to test tab url case
  })

})
