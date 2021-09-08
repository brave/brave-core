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
  contexts: ['all']
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
      const { host, selector } = msg
      applyCosmeticFilter(host, selector)
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
        if (tabs.length > 0) {
          chrome.tabs.sendMessage(tabs[0].id!, { type: 'elementPickerLaunch' })
        }
      })
      break
    }
    default: {
      console.warn('[cosmeticFilterEvents] invalid context menu option: ${info.menuItemId}')
    }
  }
}
