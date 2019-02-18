import cosmeticFilterActions from '../actions/cosmeticFilterActions'

let rule = {
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
  title: 'Block element via selector',
  id: 'addBlockElement',
  parentId: 'brave',
  contexts: ['all']
})
chrome.contextMenus.create({
  title: 'Clear CSS rules for this site',
  id: 'resetSiteFilterSettings',
  parentId: 'brave',
  contexts: ['all']
})
chrome.contextMenus.create({
  title: 'Clear CSS rules for all sites',
  id: 'resetAllFilterSettings',
  parentId: 'brave',
  contexts: ['all']
})

// contextMenu listener - when triggered, grab latest selector
chrome.contextMenus.onClicked.addListener(function (info, tab) {
  switch (info.menuItemId) {
    case 'addBlockElement':
      {
        rule.selector = window.prompt('CSS selector to block: ', `${rule.selector}`) || ''
        chrome.tabs.insertCSS({
          code: `${rule.selector} {display: none;}`
        })
        cosmeticFilterActions.siteCosmeticFilterAdded(rule.host, rule.selector)
        break
      }
    case 'resetSiteFilterSettings':
      {
        cosmeticFilterActions.siteCosmeticFilterRemoved(rule.host)
        break
      }
    case 'resetAllFilterSettings':
      {
        cosmeticFilterActions.allCosmeticFiltersRemoved()
        break
      }
    default: {
      console.warn('[cosmeticFilterEvents] invalid context menu option: ${info.menuItemId}')
    }
  }
})

// content script listener for right click DOM selection event
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  rule.host = msg.baseURI
  rule.selector = msg.selector
  sendResponse(rule)
})
