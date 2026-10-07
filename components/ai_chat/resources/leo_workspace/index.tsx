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
// requiring launchQueue again. A workspace that is created without a folder is
// never sent one: the first time one of its tools runs, the origin private file
// system (OPFS) root becomes its folder, and is stored in IndexedDB the same
// way. The file tools are registered with Leo via WebMCP (see tools.ts /
// file_ops.ts). The `view.` sibling origin can also read files over
// postMessage (see message_handler.ts).
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
// consumers can await rather than a slot they have to poll. It is never
// rejected: a workspace without a folder falls back to OPFS (see getRoot).
const { promise: rootHandle, resolve: resolveRoot } =
  Promise.withResolvers<FileSystemDirectoryHandle>()
let hasRoot = false

// Called when we have a valid directory handle. The first one wins.
function onDirectoryHandle(
  root: FileSystemDirectoryHandle,
  source: 'IndexedDB' | 'launchQueue' | 'OPFS',
) {
  if (hasRoot) {
    return
  }
  console.debug(
    '[leo-workspace] received directory handle:',
    root.name,
    `(from ${source})`,
  )
  hasRoot = true
  resolveRoot(root)

  // Store in IndexedDB for future loads (unless that's where it came from).
  if (source !== 'IndexedDB') {
    storeDirectoryHandle(root).catch((e) => {
      console.error('[leo-workspace] failed to store directory handle:', e)
    })
  }
}

// Looks for the folder the workspace already has: the one stored in IndexedDB,
// or failing that one delivered via launchQueue. setConsumer flushes a launch
// that is already queued, so a folder that has been sent is picked up here.
async function findExistingRoot() {
  const storedHandle = await restoreDirectoryHandle()
  if (storedHandle) {
    onDirectoryHandle(storedHandle, 'IndexedDB')
  } else if (window.launchQueue) {
    window.launchQueue.setConsumer(onLaunch)
  } else {
    console.error('[leo-workspace] window.launchQueue is unavailable')
  }
}
let foundExistingRoot: Promise<void>

// The root the tools run against. A workspace created without a folder is
// never sent one, so the first time a tool runs without a folder, the origin
// private file system root (private to the workspace, as each has its own
// origin) becomes the workspace folder.
async function getRoot(): Promise<FileSystemDirectoryHandle> {
  await foundExistingRoot
  if (!hasRoot) {
    onDirectoryHandle(await navigator.storage.getDirectory(), 'OPFS')
  }
  return rootHandle
}

function onLaunch(params: LaunchParams) {
  const entry = params.files?.[0]
  if (!entry || entry.kind !== 'directory') {
    console.error(
      '[leo-workspace] launch params missing a directory handle',
      params,
    )
    // Don't reject - we might have a stored handle, or fall back to OPFS.
    return
  }
  const root = entry as FileSystemDirectoryHandle
  onDirectoryHandle(root, 'launchQueue')
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

function initialize() {
  console.debug('[leo-workspace] bundle loaded at', window.location.origin)
  installMessageHandler(rootHandle)

  // Handle file viewing via hash fragment
  handleHashChange()
  window.addEventListener('hashchange', handleHashChange)

  foundExistingRoot = findExistingRoot()
  // Register the tools up front: they resolve the folder when they run, so a
  // workspace that is never sent a folder still gets them (backed by OPFS).
  void registerTools(getRoot)
}

document.addEventListener('DOMContentLoaded', initialize)
