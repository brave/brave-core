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

import { installMessageHandler } from './message_handler'
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

function initialize() {
  console.log('[leo-workspace] bundle loaded at', window.location.origin)
  installMessageHandler(rootHandle)
  if (window.launchQueue) {
    window.launchQueue.setConsumer(onLaunch)
  } else {
    console.error('[leo-workspace] window.launchQueue is unavailable')
    rejectRoot(new Error('workspace folder is unavailable'))
  }
}

document.addEventListener('DOMContentLoaded', initialize)
