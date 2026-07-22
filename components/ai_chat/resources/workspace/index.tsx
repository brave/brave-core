// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Headless workspace page. This bundle runs inside the hidden
// chrome-untrusted://workspace/<guid> WebContents that is attached to a Leo
// conversation. It receives a FileSystemDirectoryHandle for the user-picked
// folder (delivered by the browser via window.launchQueue), implements the file
// tools against it, and registers them with Leo via WebMCP
// (navigator.modelContext). There is no visible UI.
//
// The handle is delivered via launchQueue; once captured, the file tools are
// registered with Leo via WebMCP (see tools.ts / file_ops.ts).

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

let rootHandle: FileSystemDirectoryHandle | null = null

function onLaunch(params: LaunchParams) {
  const entry = params.files?.[0]
  if (!entry || entry.kind !== 'directory') {
    console.error(
      '[workspace] launch params missing a directory handle',
      params,
    )
    return
  }
  rootHandle = entry as FileSystemDirectoryHandle
  console.log('[workspace] received directory handle:', rootHandle.name)
  void registerTools(rootHandle)
}

function initialize() {
  console.log('[workspace] bundle loaded at', window.location.pathname)
  if (window.launchQueue) {
    window.launchQueue.setConsumer(onLaunch)
  } else {
    console.error('[workspace] window.launchQueue is unavailable')
  }
}

document.addEventListener('DOMContentLoaded', initialize)

export {}
