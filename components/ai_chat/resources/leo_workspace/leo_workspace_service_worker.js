// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Service worker for chrome-untrusted://leo-workspace/. Serves a workspace's
// folder at /<uuid>/files/<path> by asking that workspace's page to read the
// file with its FileSystemDirectoryHandle, and leaves every other request to
// the WebUI data source.
//
// Registered browser-side (see workspace_service_worker.h), because a WebUI
// origin cannot register a worker from JavaScript.
//
// Plain JS on purpose: it is shipped as a resource rather than bundled, so it
// stays a classic worker script with no module resolution to arrange.

'use strict'

// Keep in sync with kAIChatLeoWorkspaceFilesSegment.
const kFilesSegment = 'files'

// A page that has not answered by now is treated as gone. The read itself is
// local, so this only has to cover a page that is wedged or shutting down.
const kRequestTimeoutMs = 10000

// Extensions we are prepared to name a type for. Anything else is served as an
// attachment instead of being guessed at: the alternative is a wrong
// Content-Type, and for text/html that would mean markup in a data file being
// parsed as a document.
const kMimeTypes = new Map([
  ['html', 'text/html'],
  ['htm', 'text/html'],
  ['css', 'text/css'],
  ['js', 'text/javascript'],
  ['mjs', 'text/javascript'],
  ['json', 'application/json'],
  ['txt', 'text/plain'],
  ['md', 'text/markdown'],
  ['csv', 'text/csv'],
  ['xml', 'text/xml'],
  ['svg', 'image/svg+xml'],
  ['png', 'image/png'],
  ['jpg', 'image/jpeg'],
  ['jpeg', 'image/jpeg'],
  ['gif', 'image/gif'],
  ['webp', 'image/webp'],
  ['avif', 'image/avif'],
  ['ico', 'image/x-icon'],
  ['woff2', 'font/woff2'],
  ['woff', 'font/woff'],
  ['ttf', 'font/ttf'],
  ['mp3', 'audio/mpeg'],
  ['wav', 'audio/wav'],
  ['ogg', 'audio/ogg'],
  ['mp4', 'video/mp4'],
  ['webm', 'video/webm'],
  ['pdf', 'application/pdf'],
])

// What a served file is allowed to do. Deliberately not sandboxed: a sandboxed
// response gets an opaque origin, and an opaque origin cannot be controlled by
// a service worker, so a served page's own subresources would not reach this
// worker and nothing beyond a self-contained file could be served. Keeping
// model-authored content off the workspace page's origin is left to giving each
// workspace an origin of its own. See workspace_service_worker.h.
//
// The sources are this workspace's own files root, so a served page can load
// the rest of its folder and nothing else: not another workspace's files, and
// not the workspace page or its bundle. A source with a path matches the URL
// being requested, which is what makes that expressible.
function fileCSP(uuid) {
  const filesRoot = `${self.location.origin}/${uuid}/${kFilesSegment}/`
  return [
    `default-src ${filesRoot} 'unsafe-inline' 'unsafe-eval' data: blob:`,
    // No network at all, so a served file cannot send a folder's contents
    // anywhere.
    "connect-src 'none'",
    "form-action 'none'",
    "base-uri 'none'",
  ].join('; ')
}

// Splits "/<uuid>/files/<path>", or returns null when the request is not for a
// workspace file. Path traversal is not a concern here: the segments are handed
// to the page, which resolves them with getDirectoryHandle()/getFileHandle(),
// and those reject separators and '..' and cannot leave the folder.
function decodeSegment(segment) {
  try {
    return decodeURIComponent(segment)
  } catch {
    // Not something a browser produces, and not worth failing the request over:
    // a name that does not decode simply will not be found in the folder.
    return segment
  }
}

function parseFileRequest(url) {
  const segments = url.pathname.split('/').filter((s) => s !== '')
  if (segments.length < 3 || segments[1] !== kFilesSegment) {
    return null
  }
  return {
    uuid: segments[0],
    path: segments.slice(2).map(decodeSegment).join('/'),
  }
}

function mimeTypeFor(path) {
  const name = path.slice(path.lastIndexOf('/') + 1)
  const dot = name.lastIndexOf('.')
  return dot > 0 ? kMimeTypes.get(name.slice(dot + 1).toLowerCase()) : undefined
}

// The window clients that might hold a handle for workspace |uuid|: its page,
// but also any other tab opened on the same URL, which has no handle and will
// never answer. Uncontrolled clients count, since the page is loaded once, when
// the workspace is created, and may well predate this worker.
async function findWorkspaceClients(uuid) {
  const clients = await self.clients.matchAll({
    type: 'window',
    includeUncontrolled: true,
  })
  return clients.filter((client) => {
    const path = new URL(client.url).pathname
    return path === `/${uuid}` || path === `/${uuid}/`
  })
}

// Asks |client| for |path| over a private channel, so the reply cannot be
// spoofed by anything else that can postMessage to this worker.
function requestFile(client, path) {
  return new Promise((resolve) => {
    const channel = new MessageChannel()
    const timer = setTimeout(() => {
      channel.port1.close()
      resolve(null)
    }, kRequestTimeoutMs)

    channel.port1.onmessage = (event) => {
      clearTimeout(timer)
      channel.port1.close()
      resolve(event.data && event.data.bytes ? event.data : null)
    }
    client.postMessage({ type: 'leo-workspace-read-file', path }, [
      channel.port2,
    ])
  })
}

// Asks every candidate at once and answers with the first real reply, so a tab
// sitting on the workspace URL without a handle cannot swallow a request, nor
// make it wait out its timeout.
function readFromAnyClient(clients, path) {
  return new Promise((resolve) => {
    let pending = clients.length
    for (const client of clients) {
      void requestFile(client, path).then((reply) => {
        if (reply) {
          resolve(reply)
        } else if (--pending === 0) {
          resolve(null)
        }
      })
    }
  })
}

// Says which stage failed, because an empty response looks exactly like a
// navigation that never reached this worker.
function notFound(reason) {
  console.warn('[leo-workspace-sw]', reason)
  return new Response(`Leo workspace: ${reason}\n`, {
    status: 404,
    headers: new Headers({
      'Content-Type': 'text/plain',
      'Content-Security-Policy': "sandbox; default-src 'none'",
    }),
  })
}

async function serveWorkspaceFile(request) {
  const parsed = parseFileRequest(new URL(request.url))
  if (!parsed) {
    return null
  }
  // Only reads: a folder is not writable over its own URLs.
  if (request.method !== 'GET' && request.method !== 'HEAD') {
    return new Response(null, { status: 405 })
  }

  // Fails closed once the workspace is gone, rather than answering from
  // whichever folder happens to be open now.
  const clients = await findWorkspaceClients(parsed.uuid)
  if (clients.length === 0) {
    return notFound(`no page open for workspace ${parsed.uuid}`)
  }

  const file = await readFromAnyClient(clients, parsed.path)
  if (!file) {
    return notFound(`no workspace page could read ${parsed.path}`)
  }

  const type = mimeTypeFor(parsed.path)
  const headers = new Headers({
    'Content-Security-Policy': fileCSP(parsed.uuid),
    'Content-Type': type || 'application/octet-stream',
    'X-Content-Type-Options': 'nosniff',
    // The folder changes underneath us as the model writes to it.
    'Cache-Control': 'no-store',
  })
  if (!type) {
    // Unknown type: let the user save it rather than rendering a guess.
    headers.set('Content-Disposition', 'attachment')
  }
  return new Response(request.method === 'HEAD' ? null : file.bytes, {
    status: 200,
    headers,
  })
}

self.addEventListener('install', () => {
  // Serve as soon as this version is installed: the pages that need it are
  // already open.
  self.skipWaiting()
})

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim())
})

self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url)
  if (!parseFileRequest(url)) {
    // Everything else on this host is the workspace page and its bundle, which
    // the WebUI data source serves.
    return
  }
  event.respondWith(
    serveWorkspaceFile(event.request).then(
      (response) => response || notFound(`could not serve ${url.pathname}`),
    ),
  )
})
