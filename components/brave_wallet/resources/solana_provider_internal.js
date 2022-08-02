// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

(function () {
  if (!window._brave_solana || !!window._brave_solana.createPublickey ||
      !!window._brave_solana.createTransaction) {
    return
  }

  $Object.defineProperties(window._brave_solana, {
    createPublickey: {
      value: (base58Str) => {
        const PublicKey = require('@solana/web3.js').PublicKey
        const result = new $Object.self()
        result.publicKey = new PublicKey(base58Str)
        return result
      },
      writable: false
    },
    createTransaction: {
      value: (serializedTx) => {
        const Transaction = require('@solana/web3.js').Transaction
        return Transaction.from(new Uint8Array(serializedTx))
      },
      writable: false
    }
  })
  $Object.freeze(window._brave_solana)
})()
