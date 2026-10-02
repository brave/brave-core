// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// These names are consumed by scriptlets concatenated after this resource.
let deAmpEnabled = false

const scriptletGlobals = (() => {
  const [canDebug, isDeAmpEnabled] = JSON.parse(
    '__BRAVE_SCRIPTLET_GLOBALS_CONFIG__',
  ) as [boolean, boolean]
  deAmpEnabled = isDeAmpEnabled

  const forwardedMapMethods = new Set<PropertyKey>(['has', 'get', 'set'])
  const handler: ProxyHandler<Map<PropertyKey, unknown>> = {
    get(target, prop) {
      if (forwardedMapMethods.has(prop)) {
        return Reflect.get(target, prop).bind(target)
      }
      return target.get(prop)
    },
    set(target, prop, value) {
      if (!forwardedMapMethods.has(prop)) {
        target.set(prop, value)
      }
      return true
    },
  }

  const globals = new Map<PropertyKey, unknown>()
  if (canDebug) {
    globals.set('canDebug', true)
  }
  return new Proxy(globals, handler)
})()
