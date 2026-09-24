// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import JSDOMEnvironment from 'jest-environment-jsdom'
import assert from 'assert'

export default class CustomEnvironment extends JSDOMEnvironment {
  async setup() {
    await super.setup()

    // Treat JSDOM errors as hard failures. These errors usually occur due to
    // unimplemented APIs, so it is better to detect those and explicitly mock.
    assert(this.dom.virtualConsole.listenerCount('jsdomError') === 1)
    this.dom.virtualConsole.removeAllListeners('jsdomError')

    // We're adding features missing in JSDOM. If assertions fail, JSDOM has
    // these features now and we can remove our additions.
    assert(this.global.TextEncoder === undefined)
    this.global.TextEncoder = TextEncoder

    assert(this.global.TextDecoder === undefined)
    this.global.TextDecoder = TextDecoder

    assert(this.global.matchMedia === undefined)
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

    // structuredClone is only available in jest-environment-node and not jsdom
    // https://github.com/jestjs/jest/pull/12631
    assert(this.global.structuredClone === undefined)
    this.global.structuredClone = structuredClone
  }
}
