// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '../api/localeAPI'
import { openFilterManagementPage } from '../api/cosmeticFilterAPI'

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

chrome.contextMenus.onClicked.addListener(
  (info: chrome.contextMenus.OnClickData, tab: chrome.tabs.Tab) => {
      onContextMenuClicked(info, tab)
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
