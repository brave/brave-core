/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

self.addEventListener('install', event => {
  console.log("Service worker installed.")
  event.waitUntil(self.skipWaiting()) // Activate worker immediately
})

self.addEventListener('activate', event => {
  console.log("Service worker activated.")
  event.waitUntil(self.clients.claim()) // Become available to all pages
})

// Handle all fetches.
self.addEventListener('fetch', event => {
  async function doFetch() {
    return fetch(event.request)
    .catch((error) => {
      console.error(`Fetching failed: ${error}`)
      throw error
    })
  }
  event.respondWith(
     doFetch()
  )
})
