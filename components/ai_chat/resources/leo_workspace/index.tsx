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
// The handle is delivered via launchQueue; once captured, the file tools are
// registered with Leo via WebMCP (see tools.ts / file_ops.ts). The `view.`
// sibling origin can also read files over postMessage (see message_handler.ts).
//
// When this page's own URL fragment names a file, it is being used as a shell
// to preview it: it frames a viewer for that file, full page. Reading stays
// here, because this is the origin the folder was granted to.

import { fileFragment, filePathFromHash } from './file_fragment'
import { installMessageHandler, viewOrigin } from './message_handler'
import { registerTools } from './tools'

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

function onLaunch(params: LaunchParams) {
  const entry = params.files?.[0]
  if (!entry || entry.kind !== 'directory') {
    console.error(
      '[leo-workspace] launch params missing a directory handle',
      params,
    )
    rejectRoot(new Error('workspace folder is unavailable'))
    return
  }
  const root = entry as FileSystemDirectoryHandle
  console.log('[leo-workspace] received directory handle:', root.name)
  resolveRoot(root)
  void registerTools(root)
}

// The viewer being framed, if one is. Kept so that a retargeted fragment
// changes its URL rather than replacing the element, which keeps the framed
// document (and the worker controlling it) alive.
let viewerFrame: HTMLIFrameElement | null = null

// Frames a viewer for the file the fragment asks for, or removes the frame
// when it asks for none. |origin| is this page's own; the viewer is its
// `view.` sibling, which is the only origin this page may frame.
export function showViewerFrame(
  root: HTMLElement,
  origin = window.location.origin,
) {
  const path = filePathFromHash(window.location.hash)
  if (!path) {
    viewerFrame?.remove()
    viewerFrame = null
    return
  }
  const viewer = viewOrigin(origin)
  if (!viewer) {
    console.error('[leo-workspace] no viewer origin to frame')
    return
  }

  const src = `${viewer}/${fileFragment(path)}`
  if (viewerFrame) {
    // Same document, new fragment: the viewer re-reads without reloading.
    viewerFrame.src = src
    return
  }
  viewerFrame = document.createElement('iframe')
  viewerFrame.src = src
  viewerFrame.title = 'Leo Workspace View'
  // Set through the CSSOM rather than a style attribute, which this page's
  // style-src would block.
  const style = viewerFrame.style
  style.position = 'fixed'
  style.inset = '0'
  style.width = '100%'
  style.height = '100%'
  style.border = '0'
  root.appendChild(viewerFrame)
}

function initialize() {
  console.log('[leo-workspace] bundle loaded at', window.location.origin)
  installMessageHandler(rootHandle)
  if (window.launchQueue) {
    window.launchQueue.setConsumer(onLaunch)
  } else {
    console.error('[leo-workspace] window.launchQueue is unavailable')
    rejectRoot(new Error('workspace folder is unavailable'))
  }

  const root = document.getElementById('root')
  if (!root) {
    console.error('[leo-workspace] no #root to frame the viewer in')
    return
  }
  window.addEventListener('hashchange', () => showViewerFrame(root))
  showViewerFrame(root)
}

document.addEventListener('DOMContentLoaded', initialize)
