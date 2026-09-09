// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Answers the service worker's requests for files in this workspace's folder,
// so that a folder is served through the same FileSystemDirectoryHandle the
// tools use. See leo_workspace_service_worker.js.

import { readBytes } from './file_ops'

// A reply is one transferred buffer, so nothing streams and an oversized file
// would sit in the page's memory entire. Refused instead.
const kMaxServedFileSize = 64 * 1024 * 1024

interface ReadFileRequest {
  type: 'leo-workspace-read-file'
  path: string
}

function isReadFileRequest(data: unknown): data is ReadFileRequest {
  return (
    typeof data === 'object'
    && data !== null
    && (data as ReadFileRequest).type === 'leo-workspace-read-file'
    && typeof (data as ReadFileRequest).path === 'string'
  )
}

async function onMessage(root: FileSystemDirectoryHandle, event: MessageEvent) {
  // The worker replies down a port of its own, so a request without one did not
  // come from it.
  const port = event.ports[0]
  if (!port || !isReadFileRequest(event.data)) {
    return
  }

  try {
    const bytes = await readBytes(root, event.data.path, kMaxServedFileSize)
    port.postMessage({ bytes }, [bytes.buffer])
  } catch (error) {
    // A missing file is the common case here, and the worker turns an empty
    // reply into a 404.
    console.warn('[leo-workspace] could not serve', event.data.path, error)
    port.postMessage({})
  }
}

// Serves |root| for as long as this page lives. Nothing persists: with the page
// gone the worker has no one to ask, so the folder stops being readable over
// its URLs.
export function serveFiles(root: FileSystemDirectoryHandle) {
  const container = navigator.serviceWorker
  if (!container) {
    console.error('[leo-workspace] no service worker container to serve files')
    return
  }
  container.addEventListener('message', (event) => {
    void onMessage(root, event)
  })
  // Messages from the worker are queued until the page asks for them, and
  // addEventListener() alone does not (only assigning onmessage would).
  container.startMessages()
}
