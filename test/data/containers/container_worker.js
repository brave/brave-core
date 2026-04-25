// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Service worker for container isolation testing
self.addEventListener('install', (event) => {
  console.log('Container service worker installing')
  self.skipWaiting()
})

self.addEventListener('activate', (event) => {
  console.log('Container service worker activated')
  event.waitUntil(self.clients.claim())
})

self.addEventListener('fetch', (event) => {
  // Simple pass-through for testing
  event.respondWith(fetch(event.request))
})
