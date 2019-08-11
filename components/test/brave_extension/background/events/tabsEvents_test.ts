/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../../../../brave_extension/extension/brave_extension/background/events/tabsEvents'
import actions from '../../../../brave_extension/extension/brave_extension/background/actions/tabActions'
import * as types from '../../../../brave_extension/extension/brave_extension/constants/tabTypes'

interface TabActivatedEvent extends chrome.events.Event<(activeInfo: chrome.tabs.TabActiveInfo) => void> {
  emit: (detail: chrome.tabs.TabActiveInfo) => void
}

interface TabCreatedEvent extends chrome.events.Event<(activeInfo: chrome.tabs.Tab) => void> {
  emit: (detail: chrome.tabs.Tab) => void
}

interface TabUpdatedEvent extends chrome.events.Event<(tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => void> {
  emit: (tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => void
}

describe('tabsEvents events', () => {
  describe('chrome.tabs.onActivated', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(actions, 'activeTabChanged')
        // ensure return value is also mocked so a warning about lack of tabId is not thrown
        .mockReturnValue({ type: types.ACTIVE_TAB_CHANGED, windowId: 1, tabId: 2 })
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls actions.activeTabChanged with the correct args', (cb) => {
      const inputWindowId = 1
      const inputTabId = 2

      const onActivated = chrome.tabs.onActivated as TabActivatedEvent

      onActivated.addListener((activeInfo) => {
        expect(activeInfo.windowId).toBe(inputWindowId)
        expect(activeInfo.tabId).toBe(inputTabId)
        expect(spy).toBeCalledWith(inputWindowId, inputTabId)
        cb()
      })
      onActivated.emit({ windowId: inputWindowId, tabId: inputTabId })
    })
  })
  describe('chrome.tabs.onCreated', () => {
    let spy: jest.SpyInstance
    const inputTab: chrome.tabs.Tab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true,
      discarded: false,
      autoDiscardable: false
    }
    beforeEach(() => {
      spy = jest.spyOn(actions, 'tabCreated')
        // ensure return value is also mocked so a warning about lack of tabId is not thrown
        .mockReturnValue({ type: types.TAB_CREATED, tab: inputTab })
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls tabCreated with the correct args', (cb) => {

      const onCreated = chrome.tabs.onCreated as TabCreatedEvent

      onCreated.addListener((tab) => {
        expect(tab).toBe(inputTab)
        expect(spy).toBeCalledWith(inputTab)
        cb()
      })
      onCreated.emit(inputTab)
    })
  })

  describe('chrome.tabs.onUpdated', () => {
    let spy: jest.SpyInstance
    const inputTabId = 3
    const inputChangeInfo = {}
    const inputTab: chrome.tabs.Tab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true,
      discarded: false,
      autoDiscardable: false
    }
    beforeEach(() => {
      spy = jest.spyOn(actions, 'tabDataChanged')
        // ensure return value is also mocked so a warning about lack of tabId is not thrown
        .mockReturnValue({
          type: types.TAB_DATA_CHANGED,
          tab: inputTab,
          tabId: inputTabId,
          changeInfo: inputChangeInfo
        })
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls tabDataChanged', (cb) => {
      const onUpdated = chrome.tabs.onUpdated as TabUpdatedEvent

      onUpdated.addListener((tabId, changeInfo, tab) => {
        expect(tabId).toBe(inputTabId)
        expect(changeInfo).toBe(inputChangeInfo)
        expect(tab).toBe(inputTab)
        expect(spy).toBeCalledWith(tabId, changeInfo, tab)
        cb()
      })
      onUpdated.emit(inputTabId, inputChangeInfo, inputTab)
    })
  })
})
