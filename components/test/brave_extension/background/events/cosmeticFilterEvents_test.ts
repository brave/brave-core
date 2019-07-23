/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import cosmeticFilterActions from '../../../../brave_extension/extension/brave_extension/background/actions/cosmeticFilterActions'
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
    let resetSiteFilterSettingsSpy: jest.SpyInstance
    let resetAllFilterSettingsSpy: jest.SpyInstance
    let chromeTabsSendMessageSpy: jest.SpyInstance
    beforeEach(() => {
      contextMenuOnClickedSpy = jest.spyOn(chrome.tabs, 'create')
      chromeTabsQuerySpy = jest.spyOn(chrome.tabs, 'query')
      resetSiteFilterSettingsSpy = jest.spyOn(cosmeticFilterActions, 'siteCosmeticFilterRemoved')
      resetAllFilterSettingsSpy = jest.spyOn(cosmeticFilterActions, 'allCosmeticFiltersRemoved')
      chromeTabsSendMessageSpy = jest.spyOn(chrome.tabs, 'sendMessage')
    })
    afterEach(() => {
      contextMenuOnClickedSpy.mockRestore()
      chromeTabsSendMessageSpy.mockRestore()
    })

    describe('addBlockElement', function () {
      it('triggers addBlockElement action (query call)', function () {
        const info: chrome.contextMenus.OnClickData = { menuItemId: 'addBlockElement', editable: false, pageUrl: 'brave.com' }
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
      it('calls tabsCallback', function () {
        const myTab: chrome.tabs.Tab = {
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
        cosmeticFilterEvents.tabsCallback([myTab])
        expect(1).toBe(1)
        chrome.tabs.sendMessage(myTab.id, { type: 'getTargetSelector' }, cosmeticFilterEvents.onSelectorReturned)
      })
    })
    describe('resetSiteFilterSettings', function () {
      it('triggers `siteCosmeticFilterRemoved` action', function () {
        const info: chrome.contextMenus.OnClickData = { menuItemId: 'resetSiteFilterSettings', editable: false, pageUrl: 'brave.com' }
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
        expect(resetSiteFilterSettingsSpy).toBeCalled()
      })
    })
    describe('resetAllFilterSettings', function () {
      it('triggers `allCosmeticFiltersRemoved` action', function () {
        const info: chrome.contextMenus.OnClickData = { menuItemId: 'resetAllFilterSettings', editable: false, pageUrl: 'brave.com' }
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
        expect(resetAllFilterSettingsSpy).toBeCalled()
      })
    })
    describe('onSelectorReturned', function () {
      describe('when prompting user with selector', function () {
        describe('when a selector is returned', function () {
          it('calls window.prompt with selector as input', function () {
            cosmeticFilterEvents.onSelectorReturned('abc')
            expect(lastInputText).toBe('CSS selector:')
            expect(lastPromptText).toBe('abc')
          })
        })
        describe('when a selector is not returned', function () {
          it('calls window.prompt with `not found` message', function () {
            cosmeticFilterEvents.onSelectorReturned(null)
            expect(lastInputText.indexOf('We were unable to automatically populat') > -1).toBe(true)
          })
        })
      })
      describe('after selector prompt is shown', function () {
        let insertCssSpy: jest.SpyInstance
        beforeEach(() => {
          insertCssSpy = jest.spyOn(chrome.tabs, 'insertCSS')
        })
        afterEach(() => {
          insertCssSpy.mockRestore()
        })
        it('calls `chrome.tabs.insertCSS` when selector is NOT null/undefined', function () {
          selectorToReturn = '#test_selector'
          cosmeticFilterEvents.onSelectorReturned(selectorToReturn)
          let returnObj = {
            'code': '#test_selector {display: none;}'
          }
          expect(insertCssSpy).toBeCalledWith(returnObj)
        })
        it('does NOT call `chrome.tabs.insertCSS` when selector is undefined', function () {
          selectorToReturn = undefined
          cosmeticFilterEvents.onSelectorReturned(undefined)
          expect(insertCssSpy).not.toBeCalled()
        })
        it('does NOT call `chrome.tabs.insertCSS` when selector is null', function () {
          selectorToReturn = null
          cosmeticFilterEvents.onSelectorReturned(null)
          expect(insertCssSpy).not.toBeCalled()
        })
      })
    })
  })
})
