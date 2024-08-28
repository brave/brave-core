// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '../api/localeAPI'
import { addSiteCosmeticFilter, openFilterManagementPage } from '../api/cosmeticFilterAPI'

export let rule = {
  host: '',
  selector: ''
}

export const applyCosmeticFilter = (host: string, selector: string) => {
  if (selector) {
    const s: string = selector.trim()

    if (s.length > 0) {
      chrome.tabs.insertCSS({
        code: `${s} {display: none !important;}`,
        cssOrigin: 'user'
      }, () => {
        if (chrome.runtime.lastError) {
          console.error('[applyCosmeticFilter] tabs.insertCSS failed: ' +
            chrome.runtime.lastError.message)
        }
      })

      addSiteCosmeticFilter(host, s)
    }
  }
}

// parent menu
chrome.contextMenus.create({
  title: 'Brave',
  id: 'brave',
  contexts: ['all']
})
chrome.contextMenus.create({
  title: getLocale('elementPickerMode'),
  id: 'elementPickerMode',
  parentId: 'brave',
  contexts: ['all'],
  enabled: !chrome.extension.inIncognitoContext
})
chrome.contextMenus.create({
  title: getLocale('manageCustomFilters'),
  id: 'manageCustomFilters',
  parentId: 'brave',
  contexts: ['all']
})

chrome.contextMenus.onClicked.addListener((info: chrome.contextMenus.OnClickData, tab: chrome.tabs.Tab) => {
  onContextMenuClicked(info, tab)
})

// content script listener for events from the cosmetic filtering content script
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'contextMenuOpened': {
      rule.host = msg.baseURI
      break
    }
    case 'cosmeticFilterCreate': {
      if (sender.origin) {
        applyCosmeticFilter(new URL(sender.origin).host, msg.selector)
      }
      break
    }
  }
})

export function onContextMenuClicked (info: chrome.contextMenus.OnClickData, tab: chrome.tabs.Tab) {
  switch (info.menuItemId) {
    case 'manageCustomFilters':
      openFilterManagementPage()
      break
    case 'elementPickerMode': {
      chrome.tabs.query({ active: true, currentWindow: true }, (tabs: [chrome.tabs.Tab]) => {
          const tabId = tabs[0]?.id;
          if (tabId !== undefined) {
              chrome.scripting.executeScript({target : {tabId},
              files : [ "out/content_element_picker.bundle.js" ],
          })
        }
      })
      break
    }
    default: {
      console.warn(`[cosmeticFilterEvents] invalid context menu option: ${info.menuItemId}`)
    }
  }
}
