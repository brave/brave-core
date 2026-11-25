// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  kFileDisplayed,
  kFileDisplayFailed,
  kReadFileRequest,
  kReadFileResponse,
  workspaceOrigin,
} from './message_handler'

// Parses the #file=<filename> fragment from the URL.
export function parseFileFragment(hash: string): string | null {
  if (!hash || !hash.startsWith('#')) {
    return null
  }
  const params = new URLSearchParams(hash.slice(1))
  return params.get('file')
}

// The URL path the service worker serves |filename| from. Each segment is
// escaped so that a name containing e.g. a space, '#' or '?' reaches the
// service worker intact; it unescapes them again.
export function filePathToUrlPath(filename: string): string {
  return '/' + filename.split('/').map(encodeURIComponent).join('/')
}

// Tells the workspace whether the file it asked for has been displayed, e.g. so
// that it can be printed.
function notifyParent(message: Record<string, unknown>) {
  if (pendingParent && pendingWsOrigin) {
    pendingParent.postMessage(message, pendingWsOrigin)
  }
}

function setupParentBridge(
  swPort: MessagePort,
  parent: Window,
  wsOrigin: string,
) {
  console.debug('[leo-workspace-view] setting up parent bridge to', wsOrigin)

  swPort.onmessage = (event: MessageEvent) => {
    const data = event.data as { type: string } | null
    console.debug('[leo-workspace-view] received from SW:', data)
    if (!data) return

    if (data.type === kReadFileRequest) {
      console.debug('[leo-workspace-view] forwarding to parent:', data)
      parent.postMessage(data, wsOrigin)
    }
  }

  window.addEventListener('message', (event: MessageEvent) => {
    console.debug(
      '[leo-workspace-view] received message from',
      event.origin,
      event.data,
    )
    if (event.origin !== wsOrigin) {
      console.debug('[leo-workspace-view] ignoring message from wrong origin')
      return
    }
    const data = event.data as { type: string } | null
    if (!data || data.type !== kReadFileResponse) {
      console.debug('[leo-workspace-view] ignoring non-response message')
      return
    }
    console.debug('[leo-workspace-view] forwarding response to SW:', data)
    swPort.postMessage(data)
  })
}

async function initializeServiceWorker(
  parent: Window,
  wsOrigin: string,
): Promise<void> {
  const registration = await navigator.serviceWorker.ready

  const sw = registration.active
  if (!sw) {
    throw new Error('No active service worker')
  }

  const channel = new MessageChannel()

  setupParentBridge(channel.port1, parent, wsOrigin)

  await new Promise<void>((resolve, reject) => {
    channel.port1.onmessage = (event: MessageEvent) => {
      const data = event.data as { type: string } | null
      if (data?.type === 'READY') {
        setupParentBridge(channel.port1, parent, wsOrigin)
        resolve()
      }
    }

    sw.postMessage({ type: 'INIT' }, [channel.port2])

    setTimeout(() => {
      reject(new Error('Service worker initialization timed out'))
    }, 10000)
  })
}

// The HTTP status the document in |iframe| was served with, or null if it isn't
// known. The service worker answers a file it can't read with an error status
// and page, which loads just like a file does.
export function documentResponseStatus(
  iframe: HTMLIFrameElement,
): number | null {
  try {
    const [entry] = (iframe.contentWindow?.performance.getEntriesByType(
      'navigation',
    ) ?? []) as PerformanceNavigationTiming[]
    // 0 when the status isn't exposed.
    return entry?.responseStatus || null
  } catch {
    // The document isn't same-origin with the viewer.
    return null
  }
}

let contentIframe: HTMLIFrameElement | null = null

function displayFileInIframe(filename: string) {
  const root = document.getElementById('root')
  if (!root) {
    console.error('[leo-workspace-view] #root element not found')
    return
  }

  while (root.firstChild) {
    root.removeChild(root.firstChild)
  }

  if (!contentIframe) {
    contentIframe = document.createElement('iframe')
  }

  // Fires once the file and its subresources have loaded, or once the service
  // worker's error page has, if the file couldn't be read.
  const iframe = contentIframe
  iframe.onload = () => {
    const status = documentResponseStatus(iframe)
    if (status !== null && (status < 200 || status > 299)) {
      notifyParent({
        type: kFileDisplayFailed,
        file: filename,
        error: `The file could not be loaded (HTTP ${status})`,
      })
      return
    }
    notifyParent({ type: kFileDisplayed, file: filename })
  }
  iframe.src = filePathToUrlPath(filename)
  root.appendChild(iframe)

  console.debug('[leo-workspace-view] displaying file:', filename)
}

let serviceWorkerReady = false
let pendingParent: Window | null = null
let pendingWsOrigin: string | null = null

// Starts the viewer, served from |viewerOrigin|, for the workspace |parent|.
// Exported for tests, which can't give the page a viewer origin of its own.
export async function initialize(viewerOrigin: string, parent: Window | null) {
  console.debug('[leo-workspace-view] bundle loaded at', viewerOrigin)

  const wsOrigin = workspaceOrigin(viewerOrigin)
  if (!wsOrigin) {
    console.error('[leo-workspace-view] invalid viewer origin')
    return
  }

  if (!parent) {
    console.error('[leo-workspace-view] cannot access parent workspace')
    return
  }

  pendingParent = parent
  pendingWsOrigin = wsOrigin

  const filename = parseFileFragment(window.location.hash)
  if (!filename) {
    window.addEventListener('hashchange', handleHashChange)
    return
  }

  await displayFile(filename)
}

async function handleHashChange() {
  const filename = parseFileFragment(window.location.hash)
  if (filename) {
    await displayFile(filename)
  } else {
    contentIframe = null
  }
}

async function displayFile(filename: string) {
  if (!pendingParent || !pendingWsOrigin) {
    console.error('[leo-workspace-view] parent workspace not available')
    return
  }

  try {
    if (!serviceWorkerReady) {
      await initializeServiceWorker(pendingParent, pendingWsOrigin)
      serviceWorkerReady = true
    }

    displayFileInIframe(filename)
  } catch (error) {
    console.error('[leo-workspace-view] Failed to display file:', error)
    notifyParent({
      type: kFileDisplayFailed,
      file: filename,
      error: error instanceof Error ? error.message : String(error),
    })
  }
}

document.addEventListener('DOMContentLoaded', () =>
  initialize(
    window.location.origin,
    window.parent !== window ? window.parent : window.opener,
  ),
)
