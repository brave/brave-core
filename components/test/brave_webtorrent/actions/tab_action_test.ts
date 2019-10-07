/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_webtorrent/extension/constants/tab_types'
import * as actions from '../../../brave_webtorrent/extension/actions/tab_actions'

const tab: chrome.tabs.Tab = {
  id: 1,
  index: 1,
  pinned: false,
  highlighted: false,
  windowId: 1,
  active: true,
  incognito: false,
  selected: false,
  discarded: false,
  autoDiscardable: false
}

const changeInfo: chrome.tabs.TabChangeInfo = {
  url: 'https://test.com'
}

const tabId: number = 1

describe('tab_actions', () => {

  it('tabUpdated', () => {
    expect(actions.tabUpdated(tabId, changeInfo)).toEqual({
      type: types.TAB_UPDATED,
      meta: undefined,
      payload: { tabId, changeInfo }
    })
  })

  it('tabRemoved', () => {
    expect(actions.tabRemoved(tabId)).toEqual({
      type: types.TAB_REMOVED,
      meta: undefined,
      payload: { tabId }
    })
  })

  it('tabRetrieved', () => {
    expect(actions.tabRetrieved(tab)).toEqual({
      type: types.TAB_RETRIEVED,
      meta: undefined,
      payload: { tab }
    })
  })
})
