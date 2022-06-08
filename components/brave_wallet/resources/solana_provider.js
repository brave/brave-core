// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function () {
  if (!window.solana) {
    return
  }
  const EventEmitter = require('events')
  var SolanaEventEmitter = new EventEmitter()
  Object.defineProperties(window.solana, {
    on: {
      value: SolanaEventEmitter.on,
      writable: false
    },
    emit: {
      value: SolanaEventEmitter.emit,
      writable: false
    },
    removeListener: {
      value: SolanaEventEmitter.removeListener,
      writable: false
    },
    removeAllListeners: {
      value: SolanaEventEmitter.removeAllListeners,
      writable: false
    }
  })

  // This is to prevent window._brave_solana from being defined and set
  // non-configurable before we call our internal functions.
  window._brave_solana = {}
  Object.defineProperty(window, '_brave_solana', {
    enumerable: false,
    configurable: false,
    writable: false,
  })

})()
