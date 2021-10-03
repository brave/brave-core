// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'trezor-connect/lib/iframe/iframe'
const { setFetch } = require('trezor-link/lib/bridge/http')

// Overriding window.fetch to catch network requests in the page handler.
window.fetch = function (url, options) {
  return new Promise(async (resolve, reject) => {
    // @ts-ignore
    let payload = await window.parent.sendWithPromise('trezor-fetch', url, options)
    if ((url as string).startsWith('./data/config.js')) {
      let response = JSON.parse(payload.text)
      response["whitelist"].push({ "origin": window.location.origin, "priority": 0 })
      payload.text = JSON.stringify(response);
    }
    const response = {
      ok: payload.ok,
      text: () => { return payload.text },
      arrayBuffer: Promise.resolve(
        payload.text.split('').map(function (c: String) { return c.charCodeAt(0) })
      ),
      statusText: payload.statusText
    } as unknown as Response
    resolve(response)
  })
}

setFetch(window.fetch, false)
