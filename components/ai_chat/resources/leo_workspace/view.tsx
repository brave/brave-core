// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Viewer page bundle, served from chrome-untrusted://view.<uuid>.leo-workspace.
// It shows the one workspace file named in its URL fragment (#file=<path>) by
// navigating to where that file is served, /files/<path>, so that the browser
// renders the file itself rather than this page interpreting it.
//
// /files/ is served by this origin's service worker (sw.ts), out of the
// read-only folder handle the browser delivers here via launchQueue. This page's
// job is to put that handle where the worker can find it, and then get out of
// the way: the worker needs nothing from this page afterwards, which is why the
// file can replace it.

import { filePathFromHash } from './file_fragment'
import { fileURL, indexedDbFolderStore } from './viewer_files'

// launchQueue is not in the default TS DOM lib; declare the minimal surface we
// use. The delivered entry is the workspace folder.
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

// Until a worker controls this page, a /files/ request reaches the WebUI data
// source instead, which answers an unmatched path with this very document - and
// navigating to that would navigate again, and again.
async function workerControls(): Promise<boolean> {
  if (!navigator.serviceWorker) {
    console.error('[leo-workspace-view] navigator.serviceWorker is unavailable')
    return false
  }
  if (navigator.serviceWorker.controller) {
    return true
  }
  await new Promise<void>((resolve) => {
    navigator.serviceWorker.addEventListener(
      'controllerchange',
      () => resolve(),
      { once: true },
    )
  })
  return true
}

// Keeps the folder a launch delivered, and answers where the file the fragment
// asks for is served - or null when there is nothing to show. The folder is kept
// first, so that the worker is never asked for a file before it has something to
// read it out of.
export async function fileForLaunch(
  params: LaunchParams,
): Promise<string | null> {
  const entry = params.files?.[0]
  if (!entry || entry.kind !== 'directory') {
    console.error(
      '[leo-workspace-view] launch params missing a directory handle',
      params,
    )
    return null
  }
  try {
    await indexedDbFolderStore.save(entry as FileSystemDirectoryHandle)
  } catch (e) {
    console.error('[leo-workspace-view] could not keep the folder handle', e)
    return null
  }

  const path = filePathFromHash(window.location.hash)
  if (!path || !(await workerControls())) {
    return null
  }
  return fileURL(path)
}

function initialize() {
  console.log('[leo-workspace-view] bundle loaded at', window.location.origin)
  if (!window.launchQueue) {
    console.error('[leo-workspace-view] window.launchQueue is unavailable')
    return
  }
  // The file replaces this document, so the fragment is read once per launch
  // rather than watched for changes. The entry is replaced rather than added, so
  // that going back leaves the workspace rather than stepping through the files
  // that have been shown.
  window.launchQueue.setConsumer((params) => {
    void fileForLaunch(params).then((url) => {
      if (url) {
        window.location.replace(url)
      }
    })
  })
}

document.addEventListener('DOMContentLoaded', initialize)
