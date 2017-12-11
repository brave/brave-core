/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../../app/constants/tabTypes'
import * as actions from '../../../app/actions/tabActions'

describe('tabActions', () => {
  it('activeTabChanged', () => {
    const windowId = 1
    const tabId = 1
    assert.deepEqual(actions.activeTabChanged(windowId, tabId), {
      type: types.ACTIVE_TAB_CHANGED,
      windowId: windowId,
      tabId: tabId
    })
  })

  it('tabCreated', () => {
    const tab: chrome.tabs.Tab = {
      id: 1,
      index: 1,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: false
    }

    assert.deepEqual(actions.tabCreated(tab), {
      type: types.TAB_CREATED,
      tab
    })
  })

  it('tabDataChanged', () => {
    const tabId = 1
    const changeInfo = {}
    const tab: chrome.tabs.Tab = {
      id: 1,
      index: 1,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: false
    }
    assert.deepEqual(actions.tabDataChanged(tabId, changeInfo, tab), {
      type: types.TAB_DATA_CHANGED,
      tabId,
      changeInfo,
      tab
    })
  })
})
