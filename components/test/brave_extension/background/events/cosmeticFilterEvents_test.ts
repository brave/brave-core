/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as cosmeticFilterEvents from '../../../../brave_extension/extension/brave_extension/background/events/cosmeticFilterEvents'

let selectorToReturn: string

global.prompt = () => {
  return selectorToReturn
}

describe('cosmeticFilterEvents events', () => {
  describe('chrome.contextMenus.onClicked listener', () => {
    let contextMenuOnClickedSpy: jest.SpyInstance
    let chromeTabsQuerySpy: jest.SpyInstance
    let chromeTabsSendMessageSpy: jest.SpyInstance
    chrome.braveShields = {
      addSiteCosmeticFilter: () => { /* stub */ }
    }
    beforeEach(() => {
      contextMenuOnClickedSpy = jest.spyOn(chrome.tabs, 'create')
      chromeTabsQuerySpy = jest.spyOn(chrome.tabs, 'query')
      chromeTabsSendMessageSpy = jest.spyOn(chrome.tabs, 'sendMessage')
    })
    afterEach(() => {
      contextMenuOnClickedSpy.mockRestore()
      chromeTabsSendMessageSpy.mockRestore()
    })

    describe('addBlockElement', function () {
      it('triggers addBlockElement action (query call)', function () {
        const info: chrome.contextMenus.OnClickData = { menuItemId: 'elementPickerMode', editable: false, pageUrl: 'brave.com' }
        // calls query
        const tab: chrome.tabs.Tab = {
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
        cosmeticFilterEvents.onContextMenuClicked(info, tab)
        expect(chromeTabsQuerySpy).toBeCalled()
      })
    })
  })
})
