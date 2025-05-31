// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck

import JSDOMEnvironment from 'jest-environment-jsdom'
const assert = require('assert')
const webcrypto = require('crypto').webcrypto

class CustomEnvironment extends JSDOMEnvironment {
  async setup() {
    await super.setup()

    // Treat JSDOM errors as hard failures. These errors usually occur due to
    // unimplemented APIs, so it is better to detect those and explicitly mock.
    assert(this.dom.virtualConsole.listenerCount('jsdomError') === 1)
    this.dom.virtualConsole.removeAllListeners('jsdomError')
    this.dom.virtualConsole.addListener('jsdomError', (e) => {
      // We use @container queries in Nala but they cause an error in jsdom so
      // until https://github.com/jsdom/jsdom/issues/3597 is resolved, we
      // suppress CSS parsing errors:
      // Note: e.detail contains the CSS which fails to parse - we check it
      // includes @container to make sure we aren't suppressing other CSS errors
      if (e.type === 'css parsing' && e.detail.includes('@container')) {
        console.error(e)
        return;
      }

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

    // assert(this.global.crypto === undefined)
    this.global.crypto = webcrypto

    // https://github.com/jsdom/jsdom/issues/3363
    this.global.structuredClone = structuredClone

    this.global.matchMedia = (query) => ({
      matches: false,
      media: query,
      onchange: null,
      addListener: () => {},
      removeListener: () => {},
      addEventListener: () => {},
      removeEventListener: () => {},
      dispatchEvent: () => {}
    })
  }
}

module.exports = CustomEnvironment
