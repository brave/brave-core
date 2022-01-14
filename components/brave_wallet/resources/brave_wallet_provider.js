// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function() {
  if (!window.ethereum) {
    return
  }
  var EventEmitter = require('events')
  var BraveWeb3ProviderEventEmitter = new EventEmitter()
  window.ethereum.on = BraveWeb3ProviderEventEmitter.on
  window.ethereum.emit = BraveWeb3ProviderEventEmitter.emit
  window.ethereum.removeListener =
      BraveWeb3ProviderEventEmitter.removeListener
  window.ethereum.removeAllListeners =
      BraveWeb3ProviderEventEmitter.removeAllListeners
  window.ethereum.isMetaMask = true
  Object.defineProperty(window, 'ethereum', {
    value: new Proxy(window.ethereum, {
      get: (...args) => {
        return Reflect.get(...args)
      },
      set: (...args) => {
        return Reflect.set(...args)
      },
      deleteProperty: (target, prop) => {
        return true
      }
    }),
    enumerable: true,
    configurable: true,
    writable: true,
  })

  var alreadyLogged = false
  var logweb3Warning = () => {
    if (!alreadyLogged) {
      console.warn('You are accessing the window.web3 shim. This object is deprecated, please use window.ethereum instead.')
      alreadyLogged = true
    }
  }
  const web3Shim = {
    __isMetaMaskShim__: true,
    currentProvider: window.ethereum
  }
  const web3Proxy = new Proxy(web3Shim, {
    get: (...args) => {
      logweb3Warning()
      return Reflect.get(...args)
    },
    set: (...args) => {
      logweb3Warning()
      return Reflect.set(...args)
    }
  })
  Object.defineProperty(window, 'web3', {
    value: web3Proxy,
    enumerable: false,
    configurable: true,
    writable: true,
  })
})()
