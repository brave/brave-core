/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// For dapp detection:
// Redefine window.web3 so that when window.web3 is accessed by the page,
// we'll insert a meta header to say that this is a dapp and web3 is installed.
// If the page itself accesses window.web3 within the first 2 seconds, and the
// wallet is not installed yet, then we'll prompt to install it.
const code =
`
(function() {
  let alreadyInsertedMetaTag = false
  function __insertDappDetected() {
    if (!alreadyInsertedMetaTag) {
      const meta = document.createElement('meta')
      meta.name = 'dapp-detected'
      document.head.appendChild(meta)
      alreadyInsertedMetaTag = true
    }
  }

  if (window.hasOwnProperty('web3')) {
    if (!window.web3.currentProvider ||
        !window.web3.currentProvider.isMetaMask) {
      __insertDappDetected()
    }
  } else {
    var oldWeb3 = window.web3
    Object.defineProperty(window, 'web3', {
      configurable: true,
      set: function (val) {
        __insertDappDetected()
        oldWeb3 = val
      },
      get: function () {
        __insertDappDetected()
        return oldWeb3
      }
    })
  }
})()`

// We need this script inserted as early as possible even before the load
// We can't check if web3 exists here because this is an isolated world.
if (!document.querySelector('script[data-dapp-detection]')) {
  const scriptEl = document.createElement('script')
  scriptEl.dataset.dappDetection = ''
  scriptEl.textContent = code;
  (document.head || document.documentElement).appendChild(scriptEl)

  // If a website tries to access window.web3 within the first 2 seconds,
  // then we prompt to install Brave Crypto Wallets.
  // If a website does not try to access window.web3, then we will not prompt.
  window.setTimeout(() => {
    const isDapp = document.querySelector('meta[name="dapp-detected"]')
    if (isDapp) {
      chrome.runtime.sendMessage({
        type: 'dappAvailable'
      })
    }
  }, 2000)
}
