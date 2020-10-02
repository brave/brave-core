/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// For dapp detection:
// Redefine window.ethereum so that when window.ethereum is accessed by the page,
// we'll insert a meta header to say that this is a dapp and ethereum is installed.
// If the page itself accesses window.ethereum within the first 2 seconds, and the
// wallet is not installed yet, then we'll prompt to install it.

// Minified version of the comment block that follows
// generated with pbpaste | uglifyjs --compress --mangle | pbcopy
const code = `!function(){let e=!1;function n(){if(!e){const n=document.createElement("meta");n.name="dapp-detected",document.head.appendChild(n),e=!0}}if(window.hasOwnProperty("ethereum")){if(window.__disableDappDetectionInsertion=!0,void 0===window.ethereum)return;n()}else{var t=window.ethereum;Object.defineProperty(window,"ethereum",{configurable:!0,enumerable:!1,set:function(e){window.__disableDappDetectionInsertion||n(),t=e},get:function(){if(!window.__disableDappDetectionInsertion){const e=arguments.callee;e&&e.caller&&e.caller.toString&&-1!==e.caller.toString().indexOf("getOwnPropertyNames")||n()}return t}})}}();` // NOLINT

/*
const code = `
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

  if (window.hasOwnProperty('ethereum')) {
    // A closure can't be used for __disableDappDetectionInsertion var
    // because some sites like www.wnyc.org do a second script execution
    // via eval for some reason.
    window.__disableDappDetectionInsertion = true
    if (window.ethereum === undefined) {
      return
    }
    __insertDappDetected()
  } else {
    // Likely oldEthereum is undefined and it has a property only because
    // we defined it. Some sites like wnyc.org are evaling all scripts
    // that exist again, so this is protection against multiple calls.
    var oldEthereum = window.ethereum
    Object.defineProperty(window, 'ethereum', {
      configurable: true,
      enumerable: false,
      set: function (val) {
        if (!window.__disableDappDetectionInsertion)
          __insertDappDetected()
        oldEthereum = val
      },
      get: function () {
        if (!window.__disableDappDetectionInsertion) {
          // a special check to only detect when window.ethereum is used
          // "explicitly", instead of accessed dynamically / enumerated through
          // Object.getOwnPropertyNames
          //
          // getOwnPropertyNames will unfortunately enumerate propery names even when a property's
          // enumerable flag is false :(
          //
          // this code will admittedly check source around the call site when source is available
          // -- this is acknowledged to be a kludge.
          //
          // we want to err on the side of not showing this info bar, so adding some potential
          // "false positives" here is not necessarily a bad thing.
          //
          // also, bbondy told me to do it. <duck>
          // - mcu
          //
          const callee = arguments.callee
          if (!callee || !callee.caller || !callee.caller.toString ||
              callee.caller.toString().indexOf('getOwnPropertyNames') === -1) {
            __insertDappDetected()
          }
        }
        return oldEthereum
      }
    })
  }
})()
`*/

// We need this script inserted as early as possible even before the load
// We can't check if ethereum exists here because this is an isolated world.
if (!document.querySelector('script[data-dapp-detection]')) {
  const scriptEl = document.createElement('script')
  scriptEl.dataset.dappDetection = ''
  scriptEl.textContent = code;
  (document.head || document.documentElement).appendChild(scriptEl)

  // If a website tries to access window.ethereum within the first 2 seconds,
  // then we prompt to install Brave Crypto Wallets.
  // If a website does not try to access window.ethereum, then we will not prompt.
  window.setTimeout(() => {
    const isDapp = document.querySelector('meta[name="dapp-detected"]')
    if (isDapp) {
      chrome.runtime.sendMessage({
        type: 'dappAvailable'
      })
    }
  }, 2000)
}
