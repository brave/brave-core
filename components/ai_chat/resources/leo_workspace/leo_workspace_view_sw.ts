// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export {}

interface ExtendableEvent extends Event {
  waitUntil(promise: Promise<unknown>): void
}

interface FetchEvent extends ExtendableEvent {
  readonly request: Request
  respondWith(response: Response | Promise<Response>): void
}

interface ServiceWorkerGlobalScope {
  readonly clients: { claim(): Promise<void> }
  readonly location: { origin: string }
  skipWaiting(): Promise<void>
  addEventListener(type: 'fetch', listener: (event: FetchEvent) => void): void
  addEventListener(
    type: 'install' | 'activate',
    listener: (event: ExtendableEvent) => void,
  ): void
}

declare const self: ServiceWorkerGlobalScope

const kStaticResources = new Set([
  '/leo_workspace_view.bundle.js',
  '/leo_workspace_view_sw.bundle.js',
])

self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url)
  if (url.origin !== self.location.origin) {
    return
  }
  if (kStaticResources.has(url.pathname)) {
    return
  }
  if (url.pathname === '/' || url.pathname === '') {
    return
  }

  event.respondWith(
    new Response('<placeholder>', {
      status: 200,
      headers: { 'Content-Type': 'text/html' },
    }),
  )
})

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim())
})

self.addEventListener('install', (event) => {
  event.waitUntil(self.skipWaiting())
})
