const unique = require('unique-selector').default
let target: EventTarget | null

function getCurrentURL () {
  return window.location.hostname
}

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

// For dapp detection.
const script =
  `
  function __insertWeb3Installed() {
    if (!window.alreadyInserted) {
      const meta = document.createElement('meta')
      meta.name = 'web3-installed'
      document.head.appendChild(meta)
      window.alreadyInserted = true
    }
  }
  if (window.web3) {
    if (!window.web3.currentProvider || !window.web3.currentProvider.isMetaMask) {
      __insertWeb3Installed()
    }
  } else {
    var oldWeb3 = window.web3
    Object.defineProperty(window, 'web3', {
      configurable: true,
      set: function (val) {
        __insertWeb3Installed()
        oldWeb3 = val
      },
      get: function () {
        __insertWeb3Installed()
        return oldWeb3
      }
    })
  }`

const scriptEl = document.createElement('script')
scriptEl.textContent = script;
(document.head || document.documentElement).appendChild(scriptEl)

setTimeout(function () {
  const isDapp = document.querySelector('meta[name="web3-installed"]')
  if (isDapp) {
    chrome.runtime.sendMessage({
      type: 'dappAvailable',
      location: window.location.href
    })
    scriptEl.remove()
  }
}, 3000)
