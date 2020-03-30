import shieldsPanelActions from '../actions/shieldsPanelActions'

// content script listener for right click DOM selection event
chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
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
      const url = tab.url
      if (url === undefined) {
        break
      }
      shieldsPanelActions.contentScriptsLoaded(tabId, url)
    }
  }
})
