import { getLocale } from '../api/localeAPI'
import {
  addSiteCosmeticFilter,
  removeSiteFilter,
  removeAllFilters
} from '../api/cosmeticFilterAPI'
import shieldsPanelActions from '../actions/shieldsPanelActions'

export let rule = {
  host: '',
  selector: ''
}

// parent menu
chrome.contextMenus.create({
  title: 'Brave',
  id: 'brave',
  contexts: ['all']
})
// block ad child menu
chrome.contextMenus.create({
  title: getLocale('addBlockElement'),
  id: 'addBlockElement',
  parentId: 'brave',
  contexts: ['all']
})
chrome.contextMenus.create({
  title: getLocale('resetSiteFilterSettings'),
  id: 'resetSiteFilterSettings',
  parentId: 'brave',
  contexts: ['all']
})
chrome.contextMenus.create({
  title: getLocale('resetAllFilterSettings'),
  id: 'resetAllFilterSettings',
  parentId: 'brave',
  contexts: ['all']
})
// context menu listener emit event -> query -> tabsCallback -> onSelectorReturned

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
    case 'hiddenClassIdSelectors': {
      const tab = sender.tab
      if (tab === undefined) {
        break
      }
      const tabId = tab.id
      if (tabId === undefined) {
        break
      }
      shieldsPanelActions.generateClassIdStylesheet(tabId, msg.classes, msg.ids)
      break
    }
    case 'contentScriptsLoaded': {
      const tab = sender.tab
      if (tab === undefined) {
        break
      }
      const tabId = tab.id
      if (tabId === undefined) {
        break
      }
      const url = msg.location.href
      if (url === undefined) {
        break
      }
      const frameId = sender.frameId
      if (frameId === undefined) {
        break
      }
      shieldsPanelActions.contentScriptsLoaded(tabId, frameId, url)
    }
  }
})

export function onContextMenuClicked (info: chrome.contextMenus.OnClickData, tab: chrome.tabs.Tab) {
  switch (info.menuItemId) {
    case 'addBlockElement':
      query()
      break
    case 'resetSiteFilterSettings': {
      removeSiteFilter(rule.host)
      break
    }
    case 'resetAllFilterSettings': {
      removeAllFilters()
      break
    }
    default: {
      console.warn('[cosmeticFilterEvents] invalid context menu option: ${info.menuItemId}')
    }
  }
}

export function query () {
  chrome.tabs.query({ active: true, currentWindow: true }, (tabs: [chrome.tabs.Tab]) => {
    tabsCallback(tabs)
  })
}

export function tabsCallback (tabs: any) {
  chrome.tabs.sendMessage(tabs[0].id, { type: 'getTargetSelector' }, onSelectorReturned)
}

export async function onSelectorReturned (response: any) {
  if (!response) {
    rule.selector = window.prompt('We were unable to automatically populate a correct CSS selector for you. Please manually enter a CSS selector to block:') || ''
  } else {
    rule.selector = window.prompt('CSS selector:', `${response}`) || ''
  }

  if (rule.selector && rule.selector.length > 0) {
    chrome.tabs.insertCSS({
      code: `${rule.selector} {display: none !important;}`,
      cssOrigin: 'user'
    })
  }

  await addSiteCosmeticFilter(rule.host, rule.selector)
}
