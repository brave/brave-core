// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Headless workspace page. This bundle runs inside the hidden
// chrome-untrusted://<guid>.leo-workspace WebContents that is attached to a Leo
// conversation. Each workspace has its own subdomain, and therefore its own
// origin, so no state is shared between workspaces. It receives a
// FileSystemDirectoryHandle for the user-picked folder (delivered by the browser
// via window.launchQueue), implements the file tools against it, and registers
// them with Leo via WebMCP (navigator.modelContext). There is no visible UI.
//
// The handle is delivered via launchQueue; once captured, it's stored in
// IndexedDB so the workspace can restore it on subsequent loads without
// requiring launchQueue again. The file tools are registered with Leo via
// WebMCP (see tools.ts / file_ops.ts). The `view.` sibling origin can also
// read files over postMessage (see message_handler.ts).
//
// When navigated to with a #file=<path> fragment, the workspace creates a
// fullscreen iframe to the viewer origin which displays the file content.

import { installMessageHandler, viewOrigin } from './message_handler'
import { registerTools } from './tools'
import { restoreDirectoryHandle, storeDirectoryHandle } from './storage'

// launchQueue is not in the default TS DOM lib; declare the minimal surface we
// use. The delivered file entries are FileSystemDirectoryHandle objects.
interface LaunchParams {
  files: FileSystemHandle[]
}
interface LaunchQueue {
  setConsumer(consumer: (params: LaunchParams) => void): void
}
declare global {
  interface Window {
    launchQueue?: LaunchQueue
  }
}

// The directory handle arrives asynchronously, so expose it as a promise that
// consumers can await rather than a slot they have to poll.
const {
  promise: rootHandle,
  resolve: resolveRoot,
  reject: rejectRoot,
} = Promise.withResolvers<FileSystemDirectoryHandle>()
// Nothing awaits it until a request arrives, so don't let a failed launch
// surface as an unhandled rejection. Awaiting consumers still see it.
rootHandle.catch(() => {})

// Called when we have a valid directory handle (from launchQueue or IndexedDB).
function onDirectoryHandle(
  root: FileSystemDirectoryHandle,
  fromStorage: boolean,
) {
  console.debug(
    '[leo-workspace] received directory handle:',
    root.name,
    fromStorage ? '(from IndexedDB)' : '(from launchQueue)',
  )
  resolveRoot(root)
  void registerTools(root)

  // Store in IndexedDB for future loads (only if from launchQueue)
  if (!fromStorage) {
    void storeDirectoryHandle(root)
  }
}

function onLaunch(params: LaunchParams) {
  const entry = params.files?.[0]
  if (!entry || entry.kind !== 'directory') {
    console.error(
      '[leo-workspace] launch params missing a directory handle',
      params,
    )
    // Don't reject yet - we might have a stored handle
    return
  }
  const root = entry as FileSystemDirectoryHandle
  onDirectoryHandle(root, false)
}

// Parses the #file=<filename> fragment from the URL.
function parseFileFragment(hash: string): string | null {
  if (!hash || !hash.startsWith('#')) {
    return null
  }
  const params = new URLSearchParams(hash.slice(1))
  return params.get('file')
}

// The viewer iframe, created on demand when a #file= fragment is present.
let viewerIframe: HTMLIFrameElement | null = null

// Creates or updates the fullscreen viewer iframe.
function showViewer(filename: string) {
  const viewerOriginUrl = viewOrigin(window.location.origin)
  if (!viewerOriginUrl) {
    console.error('[leo-workspace] cannot determine viewer origin')
    return
  }

  const viewerUrl = `${viewerOriginUrl}/#file=${encodeURIComponent(filename)}`

  if (!viewerIframe) {
    viewerIframe = document.createElement('iframe')
    document.body.appendChild(viewerIframe)
  }

  viewerIframe.src = viewerUrl
  console.debug('[leo-workspace] showing viewer for:', filename)
}

// Hides the viewer iframe.
function hideViewer() {
  if (viewerIframe) {
    viewerIframe.remove()
    viewerIframe = null
  }
}

// Handles hash changes to show/hide the viewer.
function handleHashChange() {
  const filename = parseFileFragment(window.location.hash)
  if (filename) {
    showViewer(filename)
  } else {
    hideViewer()
  }
}

async function initialize() {
  console.debug('[leo-workspace] bundle loaded at', window.location.origin)
  installMessageHandler(rootHandle)

  // Handle file viewing via hash fragment
  handleHashChange()
  window.addEventListener('hashchange', handleHashChange)

  // Try to restore directory handle from IndexedDB first
  const storedHandle = await restoreDirectoryHandle()
  if (storedHandle) {
    onDirectoryHandle(storedHandle, true)
    return
  }

  // Fall back to launchQueue
  if (window.launchQueue) {
    window.launchQueue.setConsumer(onLaunch)
  } else {
    console.error('[leo-workspace] window.launchQueue is unavailable')
    rejectRoot(new Error('workspace folder is unavailable'))
  }
}

document.addEventListener('DOMContentLoaded', initialize)
