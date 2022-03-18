// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function () {
  if (!window.solana) {
    return
  }
  const solanaWeb3 = require('@solana/web3.js')
  window.solana.createPublickey = function createPublickey(base58Str) {
    const result = new Object()
    result.publicKey = new solanaWeb3.PublicKey(base58Str)
    return result
  }
  window.solana.createTransaction = function createTransaction(serializedTx) {
    return solanaWeb3.Transaction.from(new Uint8Array(serializedTx))
  }
  const EventEmitter = require('events')
  var SolanaEventEmitter = new EventEmitter()
  window.solana.on = SolanaEventEmitter.on
  window.solana.emit = SolanaEventEmitter.emit
  window.solana.removeListener = SolanaEventEmitter.removeListener
  window.solana.removeAllListeners = SolanaEventEmitter.removeAllListeners
})()
