// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { kReadFileRequest, kReadFileResponse } from './message_handler'

const kScheme = 'chrome-untrusted://'

// chrome-untrusted://view.<uuid>.leo-workspace -> chrome-untrusted://<uuid>.leo-workspace
export function workspaceOrigin(viewerOrigin: string): string {
  const viewPrefix = `${kScheme}view.`
  return viewerOrigin.startsWith(viewPrefix)
    ? viewerOrigin.replace(viewPrefix, kScheme)
    : ''
}

// Parses the #file=<filename> fragment from the URL.
export function parseFileFragment(hash: string): string | null {
  if (!hash || !hash.startsWith('#')) {
    return null
  }
  const params = new URLSearchParams(hash.slice(1))
  return params.get('file')
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

  contentIframe.src = `/${filename}`
  root.appendChild(contentIframe)

  console.debug('[leo-workspace-view] displaying file:', filename)
}

let serviceWorkerReady = false
let pendingParent: Window | null = null
let pendingWsOrigin: string | null = null

async function initialize() {
  console.debug('[leo-workspace-view] bundle loaded at', window.location.origin)

  const wsOrigin = workspaceOrigin(window.location.origin)
  if (!wsOrigin) {
    console.error('[leo-workspace-view] invalid viewer origin')
    return
  }

  const parent = window.parent !== window ? window.parent : window.opener
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
  }
}

document.addEventListener('DOMContentLoaded', initialize)
