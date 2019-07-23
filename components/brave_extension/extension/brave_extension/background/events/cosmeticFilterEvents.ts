import cosmeticFilterActions from '../actions/cosmeticFilterActions'
import { getLocale } from '../api/localeAPI'

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

// content script listener for right click DOM selection event
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'contextMenuOpened': {
      rule.host = msg.baseURI
      break
    }
  }
})

export function onContextMenuClicked (info: chrome.contextMenus.OnClickData, tab: chrome.tabs.Tab) {
  switch (info.menuItemId) {
    case 'addBlockElement':
      query()
      break
    case 'resetSiteFilterSettings': {
      cosmeticFilterActions.siteCosmeticFilterRemoved(rule.host)
      break
    }
    case 'resetAllFilterSettings': {
      cosmeticFilterActions.allCosmeticFiltersRemoved()
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

export function onSelectorReturned (response: any) {
  if (!response) {
    rule.selector = window.prompt('We were unable to automatically populate a correct CSS selector for you. Please manually enter a CSS selector to block:') || ''
  } else {
    rule.selector = window.prompt('CSS selector:', `${response}`) || ''
  }

  if (rule.selector && rule.selector.length > 0) {
    chrome.tabs.insertCSS({
      code: `${rule.selector} {display: none;}`
    })
    cosmeticFilterActions.siteCosmeticFilterAdded(rule.host, rule.selector)
  }
}
