/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as cosmeticFilterEvents from '../../../../brave_extension/extension/brave_extension/background/events/cosmeticFilterEvents'

let lastInputText: string
let lastPromptText: string
let selectorToReturn: string

global.prompt = (inputText: string, promptText: string) => {
  lastInputText = inputText
  lastPromptText = promptText
  return selectorToReturn
}

describe('cosmeticFilterEvents events', () => {
  describe('when runtime.onMessage is received', () => {
    describe('contextMenuOpened', () => {
      it('assigns the base URI', () => {
        chrome.runtime.sendMessage({ type: 'contextMenuOpened', baseURI: 'brave.com' },
        () => {
          expect(cosmeticFilterEvents.rule.host).toBe('brave.com')
        })
      })
    })
  })

  describe('chrome.contextMenus.onClicked listener', () => {
    let contextMenuOnClickedSpy: jest.SpyInstance
    let chromeTabsQuerySpy: jest.SpyInstance
    let chromeTabsSendMessageSpy: jest.SpyInstance
    let chromeBraveShieldsAddSiteCosmeticFilterSpy: jest.SpyInstance
    chrome.braveShields = {
      addSiteCosmeticFilter: () => { /* stub */ }
    }
    beforeEach(() => {
      contextMenuOnClickedSpy = jest.spyOn(chrome.tabs, 'create')
      chromeTabsQuerySpy = jest.spyOn(chrome.tabs, 'query')
      chromeTabsSendMessageSpy = jest.spyOn(chrome.tabs, 'sendMessage')
      chromeBraveShieldsAddSiteCosmeticFilterSpy = jest.spyOn(chrome.braveShields, 'addSiteCosmeticFilter')
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
    describe('onSelectorReturned', function () {
      describe('after selector prompt is shown', function () {
        let insertCssSpy: jest.SpyInstance
        beforeEach(() => {
          insertCssSpy = jest.spyOn(chrome.tabs, 'insertCSS')
        })
        afterEach(() => {
          insertCssSpy.mockRestore()
        })
        it('calls `chrome.tabs.insertCSS` with cosmetic filter rule', function () {
          selectorToReturn = '#test_selector'
          cosmeticFilterEvents.applyCosmeticFilter('brave.com', selectorToReturn)
          let returnObj = {
            'code': '#test_selector {display: none !important;}',
            'cssOrigin': 'user'
          }
          expect(insertCssSpy).toBeCalledWith(returnObj, expect.any(Function))
        })
      })
    })
  })
})
