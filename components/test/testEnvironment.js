// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const Environment = require("jest-environment-jsdom")

module.exports = class CustomEnvironment extends Environment {
  constructor(config) {
    config = Object.assign({}, config, {
      globals: Object.assign({}, config.globals, {
        // Note(petemill): Defining the Uint8Array global even though we're in a 'web' environment
        // overcomes the issue where `Buffer.from('hello') instanceof Uint8Array === false`.
        // This is needed by webtorrent dependency k-bucket.
        // https://github.com/facebook/jest/issues/6248
        // If removing these globals results in all tests passing on all platforms,
        // then we can keep them removed.
        Uint8Array: Uint8Array
      }),
    })
    super(config)
  }
}
