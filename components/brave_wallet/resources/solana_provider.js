// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

(function () {
  if (!window.braveSolana) {
    return
  }
  const EventEmitter = require('events')
  var SolanaEventEmitter = new EventEmitter()
  $Object.defineProperties(window.braveSolana, {
    on: {
      value: SolanaEventEmitter.on,
      writable: false
    },
    off: {
      value: SolanaEventEmitter.off,
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
})()
