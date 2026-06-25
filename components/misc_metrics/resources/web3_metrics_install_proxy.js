// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

function installProxy(cb) {
  function wrap(target) {
    // Only objects and functions can be proxied; primitives pass through.
    if (
      target === null
      || (typeof target !== 'object' && typeof target !== 'function')
    ) {
      return target
    }
    return new Proxy(target, {
      get: function () {
        cb()
        return wrap(Reflect.get(...arguments))
      },
      apply: function () {
        cb()
        return wrap(Reflect.apply(...arguments))
      },
    })
  }

  ;['ethereum', 'cardano', 'solana', 'web3'].forEach(function (name) {
    var existing = Object.getOwnPropertyDescriptor(window, name)
    // Don't try to redefine a non-configurable property
    if (existing && !existing.configurable) {
      return
    }
    var current = window[name]
    var stored = current === undefined ? undefined : wrap(current)
    function define(enumerable) {
      Object.defineProperty(window, name, {
        configurable: true,
        enumerable: enumerable,
        get: function () {
          return stored
        },
        set: function (value) {
          stored = wrap(value)
          // Hidden until something is actually assigned; then expose it.
          if (!enumerable) {
            define(true)
          }
        },
      })
    }
    // Start hidden if nothing is set yet, otherwise enumerable.
    define(current !== undefined)
  })

  // EIP-6963 provider discovery request, dispatched by dapps.
  window.addEventListener('eip6963:requestProvider', cb)
}
