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
  const cmd = event.data['cmd']

  if (cmd == 'fetch') {
    fetch('/simple.html')
      .then((r) => r.text())
      .then((_) => {
        event.source.postMessage({ cmd, result: 'LOADED' })
      })
      .catch((_) => {
        event.source.postMessage({ cmd, result: 'FAILED' })
      })
  }

  if (cmd == 'hasGpc') {
    event.source.postMessage({
      cmd,
      result: navigator.globalPrivacyControl !== undefined
    })
  }

  if (cmd == 'checkGpc') {
    navigator.globalPrivacyControl = false
    event.source.postMessage({
      cmd,
      result: navigator.globalPrivacyControl
    })
  }
})
