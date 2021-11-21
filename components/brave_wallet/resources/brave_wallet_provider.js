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
  // For webcompat
  window.ethereum.isMetaMask = true
})()
