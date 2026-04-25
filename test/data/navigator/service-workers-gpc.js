/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

self.addEventListener('install', function (event) {
  event.waitUntil(self.skipWaiting())
})

self.addEventListener('activate', function (event) {
  event.waitUntil(self.clients.claim())
})

self.addEventListener('message', (event) => {
  const handler = async () => {
    try {
      event.ports[0].postMessage({ data: await handleMessage(...event.data) })
    } catch (error) {
      event.ports[0].postMessage({ error })
    }
  }
  event.waitUntil(handler())
})

async function handleMessage(...args) {
  const cmd = args[0]
  if (cmd == 'fetch') {
    const r = await fetch('/simple.html')
    await r.text()
    return 'LOADED'
  }

  if (cmd == 'hasGpc') {
    return navigator.globalPrivacyControl !== undefined
  }

  if (cmd == 'checkGpc') {
    navigator.globalPrivacyControl = false
    return navigator.globalPrivacyControl
  }
}
