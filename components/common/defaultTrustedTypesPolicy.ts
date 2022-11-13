// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Most WebUI (perhaps all by the time you're reading this 🎉)
// enabled trusted types, and Brave's WebUI specifically allows
// for a `default` policy so that we can lazy-load
// JS modules.
// Import this module on any webui which you wish to call `import()` in.
// Do NOT add on to this a `createHTML` function which would have the ability
// for any script to set `HTMLElement.innerHTML`.

// @ts-expect-error
window.trustedTypes.createPolicy('default', {
  createScriptURL: (url: string) => {
    const parsed = new URL(url, document.location.href)
    const isSameOrigin = parsed.origin === document.location.origin
    if (!isSameOrigin) {
      throw new Error(`Asked for a script url that has a disallowed origin of ${parsed.origin}. URL was: ${url}.`)
    }
    return url
  }
})
