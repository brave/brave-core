// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { kReadFileRequest, kReadFileResponse } from './message_handler'

interface ExtendableEvent extends Event {
  waitUntil(promise: Promise<unknown>): void
}

interface FetchEvent extends ExtendableEvent {
  readonly request: Request
  respondWith(response: Response | Promise<Response>): void
}

interface ExtendableMessageEvent extends ExtendableEvent {
  readonly data: unknown
  readonly ports: readonly MessagePort[]
}

interface ServiceWorkerGlobalScope {
  readonly clients: { claim(): Promise<void> }
  readonly location: { origin: string }
  skipWaiting(): Promise<void>
  addEventListener(type: 'fetch', listener: (event: FetchEvent) => void): void
  addEventListener(
    type: 'message',
    listener: (event: ExtendableMessageEvent) => void,
  ): void
  addEventListener(
    type: 'install' | 'activate',
    listener: (event: ExtendableEvent) => void,
  ): void
}

declare const self: ServiceWorkerGlobalScope

let parentPort: MessagePort | null = null

const pendingRequests = new Map<
  string,
  {
    resolve: (response: Response) => void
    reject: (error: Error) => void
  }
>()

const kStaticResources = new Set([
  '/leo_workspace_view.bundle.js',
  '/leo_workspace_view_sw.bundle.js',
])

function handleInit(port: MessagePort) {
  parentPort = port
  parentPort.onmessage = handleParentMessage
  console.debug('[leo-workspace-view-sw] initialized with parent port')
}

function handleParentMessage(event: MessageEvent) {
  const data = event.data as {
    type: string
    requestId?: string
    file?: File
    error?: string
  } | null

  if (!data || data.type !== kReadFileResponse) {
    return
  }

  const requestId = data.requestId
  if (!requestId || typeof requestId !== 'string') {
    return
  }

  const pending = pendingRequests.get(requestId)
  if (!pending) {
    console.warn('[leo-workspace-view-sw] no pending request for', requestId)
    return
  }

  pendingRequests.delete(requestId)

  if (data.error) {
    pending.resolve(
      new Response(data.error, {
        status: 404,
        statusText: 'Not Found',
        headers: { 'Content-Type': 'text/plain' },
      }),
    )
  } else if (data.file) {
    const file = data.file
    pending.resolve(
      new Response(file, {
        status: 200,
        statusText: 'OK',
        headers: {
          'Content-Type': file.type || 'application/octet-stream',
          'Content-Length': String(file.size),
          'X-Content-Type-Options': 'nosniff',
        },
      }),
    )
  } else {
    pending.resolve(
      new Response('No file in response', {
        status: 500,
        statusText: 'Internal Server Error',
        headers: { 'Content-Type': 'text/plain' },
      }),
    )
  }
}

function requestFileFromParent(path: string): Promise<Response> {
  return new Promise((resolve) => {
    if (!parentPort) {
      resolve(
        new Response('Service worker not initialized', {
          status: 503,
          statusText: 'Service Unavailable',
          headers: { 'Content-Type': 'text/plain' },
        }),
      )
      return
    }

    const requestId = crypto.randomUUID()
    pendingRequests.set(requestId, { resolve, reject: () => {} })

    parentPort.postMessage({
      type: kReadFileRequest,
      requestId,
      path,
    })

    setTimeout(() => {
      if (pendingRequests.has(requestId)) {
        pendingRequests.delete(requestId)
        resolve(
          new Response('Request timed out', {
            status: 504,
            statusText: 'Gateway Timeout',
            headers: { 'Content-Type': 'text/plain' },
          }),
        )
      }
    }, 30000)
  })
}

self.addEventListener('message', (event) => {
  const data = event.data as { type: string } | null
  if (!data) return

  if (data.type === 'INIT' && event.ports[0]) {
    handleInit(event.ports[0])
    event.ports[0].postMessage({ type: 'READY' })
  }
})

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

  const filePath = url.pathname.slice(1)
  if (!filePath) {
    return
  }

  console.debug('[leo-workspace-view-sw] intercepting fetch for', filePath)
  event.respondWith(requestFileFromParent(filePath))
})

self.addEventListener('activate', (event) => {
  console.debug('[leo-workspace-view-sw] activated')
  event.waitUntil(self.clients.claim())
})

self.addEventListener('install', (event) => {
  console.debug('[leo-workspace-view-sw] installed')
  event.waitUntil(self.skipWaiting())
})
