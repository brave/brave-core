/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../../../../app/background/events/tabsEvents'
import actions from '../../../../app/background/actions/tabActions'

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
    const inputTab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true
    }
    beforeEach(() => {
      spy = jest.spyOn(actions, 'tabCreated')
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
    const inputTab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true
    }
    beforeEach(() => {
      spy = jest.spyOn(actions, 'tabDataChanged')
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
