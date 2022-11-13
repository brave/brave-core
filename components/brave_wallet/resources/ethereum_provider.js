// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

(function() {
  if (!window.ethereum) {
    return
  }
  var EventEmitter = require('events')
  var BraveWeb3ProviderEventEmitter = new EventEmitter()
  $Object.defineProperties(window.ethereum, {
    on: {
      value: BraveWeb3ProviderEventEmitter.on,
      writable: false
    },
    emit: {
      value: BraveWeb3ProviderEventEmitter.emit,
      writable: false
    },
    removeListener: {
      value: BraveWeb3ProviderEventEmitter.removeListener,
      writable: false
    },
    removeAllListeners: {
      value: BraveWeb3ProviderEventEmitter.removeAllListeners,
      writable: false
    }
  })
})()
