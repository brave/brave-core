// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const assert = require('assert')
const JSDOMEnvironment = require('jest-environment-jsdom')
const webcrypto = require('crypto').webcrypto

module.exports = class CustomEnvironment extends JSDOMEnvironment {
  async setup() {
    await super.setup()

    // Treat JSDOM errors as hard failures. These errors usually occur due to
    // unimplemented APIs, so it is better to detect those and explicitly mock.
    assert(this.dom.virtualConsole.listenerCount('jsdomError') === 1)
    this.dom.virtualConsole.removeAllListeners('jsdomError')
    this.dom.virtualConsole.addListener('jsdomError', (e) => {
      throw e
    })

    // Redefining the Uint8Array global even though we're in a 'web' environment
    // overcomes the issue where `Buffer.from('hello') instanceof Uint8Array ===
    // false`. This is needed by webtorrent dependency k-bucket.
    // https://github.com/facebook/jest/issues/6248
    //
    // If removing this override results in all tests passing on all platforms,
    // then we can keep them removed.
    this.global.Uint8Array = Uint8Array

    // We're adding features missing in JSDOM. If assertions fail, JSDOM has
    // these features now and we can remove our additions.
    assert(this.global.TextEncoder === undefined)
    this.global.TextEncoder = TextEncoder

    assert(this.global.TextDecoder === undefined)
    this.global.TextDecoder = TextDecoder

    assert(this.global.crypto === undefined)
    this.global.crypto = webcrypto
  }
}
