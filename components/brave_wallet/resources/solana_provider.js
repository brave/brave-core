// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function () {
  if (!window.solana) {
    return
  }
  const solanaWeb3 = require('@solana/web3.js')
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
    },
    createPublickey: {
      value: (base58Str) => {
        console.warn('This API is intended for internal use.')
        const result = new Object()
        result.publicKey = new solanaWeb3.PublicKey(base58Str)
        return result
      },
      writable: false
    },
    createTransaction: {
      value: (serializedTx) => {
        console.warn('This API is intended for internal use.')
        return solanaWeb3.Transaction.from(new Uint8Array(serializedTx))
      },
      writable: false
    }
  })
})()
