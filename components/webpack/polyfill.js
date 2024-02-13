// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const { ProvidePlugin } = require('webpack')

module.exports = {
  // NodeJS polyfills for the browser - these are only included when needed.
  fallback: {
    stream: require.resolve("stream-browserify"),
    path: require.resolve("path-browserify"),
    querystring: require.resolve("querystring-es3"),
    crypto: require.resolve("crypto-browserify"),
    os: require.resolve("os-browserify/browser"),
    zlib: require.resolve("browserify-zlib"),
    fs: false,
    http: require.resolve("stream-http"),
    https: require.resolve("https-browserify"),
    timers: require.resolve('timers-browserify'),
    buffer: require.resolve('buffer'),
    process: require.resolve('process/browser'),
  },
  // Provide globals from NodeJS polyfills
  provideNodeGlobals: new ProvidePlugin({
    'Buffer': ['buffer', 'Buffer'],
    'process': 'process/browser.js'
  })
}
