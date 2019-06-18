const unique = require('unique-selector').default

let target: EventTarget | null

function getCurrentURL () {
  return window.location.hostname
}

/*function applyCosmeticFilterMutationObserver (filterList: any) {
  let targetNode = document.documentElement
  let observer = new MutationObserver(function (mutations) {
    console.log('mutation observed')
    injectIncrementalStyles(mutations)
  })
  let observerConfig = {
    childList: true,
    subtree: true
    // characterData: true
  }
  observer.observe(targetNode, observerConfig)
}*/

document.addEventListener('contextmenu', (event) => {
  // send host and store target
  // `target` needed for when background page handles `addBlockElement`
  target = event.target
  chrome.runtime.sendMessage({
    type: 'contextMenuOpened',
    baseURI: getCurrentURL()
  })
}, true)

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'getTargetSelector': {
      sendResponse(unique(target))
    }
  }
})
