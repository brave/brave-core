// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Service worker for the viewer origin
// (chrome-untrusted://view.<uuid>.leo-workspace). It serves the workspace's
// files under /files/<path>, which is where the viewer page navigates to show
// one, so that the browser renders the file itself rather than a page
// interpreting it.
//
// It reads the files itself, out of the read-only folder handle the browser
// delivered to the viewer page (see WorkspaceAssociatedContent) and which that
// page left in storage. Nothing has to be asked for a file, so a served document
// - which is a client of this worker, being same-origin - has its own requests
// served the same way: its stylesheet, its images, a reload.
//
// The worker is registered browser-side in LeoWorkspaceViewUI: a
// chrome-untrusted origin cannot register one from JavaScript.

import { kFileResponseHeaders, serveFile } from './viewer_file_server'
import { indexedDbFolderStore } from './viewer_files'

// The DOM lib doesn't cover service workers; declare the minimal surface used.
interface ExtendableEvent {
  waitUntil(promise: Promise<unknown>): void
}
interface FetchEvent {
  request: Request
  respondWith(response: Promise<Response> | Response): void
}
interface WorkerClients {
  claim(): Promise<void>
}
interface WorkerScope {
  skipWaiting(): Promise<void>
  clients: WorkerClients
  addEventListener(
    type: 'install' | 'activate',
    listener: (event: ExtendableEvent) => void,
  ): void
  addEventListener(type: 'fetch', listener: (event: FetchEvent) => void): void
}

// The worker global is `self`, but that name belongs to the DOM Window type
// here; alias the worker surface to it.
const scope = self as unknown as WorkerScope

scope.addEventListener('install', (event) => {
  event.waitUntil(scope.skipWaiting())
})

scope.addEventListener('activate', (event) => {
  event.waitUntil(scope.clients.claim())
})

async function respond(url: URL): Promise<Response> {
  const served = await serveFile(url, indexedDbFolderStore)
  // serveFile() only declines what it was never asked for, and the fetch
  // handler has already checked that.
  if (!served) {
    return new Response('not a workspace file', { status: 404 })
  }
  return new Response(served.body, {
    status: served.status,
    headers: {
      'content-type': served.contentType,
      ...kFileResponseHeaders,
    },
  })
}

scope.addEventListener('fetch', (event) => {
  if (event.request.method !== 'GET') {
    return
  }
  const url = new URL(event.request.url)
  if (!url.pathname.startsWith('/files/')) {
    return
  }
  event.respondWith(respond(url))
})
