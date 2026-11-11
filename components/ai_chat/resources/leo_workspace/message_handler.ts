// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// postMessage bridge that lets the `view.` sibling origin read files out of the
// workspace folder, and lets the AI Chat page download the whole folder as a
// zip. Only those origins are accepted, each for its own request type: the
// workspace holds an auto-granted File System Access handle to a user-picked
// folder.

import {
  DownloadWorkspaceResponse,
  kAIChatOrigin,
  kDownloadWorkspaceRequest,
  kDownloadWorkspaceResponse,
} from './download'
import { readFile, zipWorkspace } from './file_ops'

export const kReadFileRequest = 'READ_FILE'
export const kReadFileResponse = 'READ_FILE_RESULT'

interface ReadFileResponse {
  type: typeof kReadFileResponse
  requestId?: unknown
  // A File (i.e. a Blob), so arbitrary binary content survives and a large file
  // isn't copied into the message.
  file?: File
  error?: string
}

const kScheme = 'chrome-untrusted://'

// chrome-untrusted://leo-workspace -> chrome-untrusted://view.leo-workspace.
// Derived at runtime so it follows the workspace host, and '' for anything that
// isn't a chrome-untrusted origin, which must never be trusted.
export function viewOrigin(origin: string): string {
  return origin.startsWith(kScheme)
    ? origin.replace(kScheme, `${kScheme}view.`)
    : ''
}

// Waits for the directory handle, with a timeout so a request doesn't hang
// forever when the workspace never gets a folder.
function awaitRoot(
  root: Promise<FileSystemDirectoryHandle>,
): Promise<FileSystemDirectoryHandle> {
  const timeoutPromise = new Promise<never>((_resolve, reject) => {
    setTimeout(
      () => reject(new Error('Workspace folder not available (timeout)')),
      5000,
    )
  })
  return Promise.race([root, timeoutPromise])
}

function errorMessage(e: unknown): string {
  return e instanceof Error ? e.message : String(e)
}

async function readFilePayload(
  root: Promise<FileSystemDirectoryHandle>,
  path: unknown,
): Promise<{ file?: File; error?: string }> {
  // Checked before awaiting the handle so a malformed request fails fast.
  if (typeof path !== 'string' || path === '') {
    return { error: 'READ_FILE requires a non-empty string "path"' }
  }
  try {
    const handle = await awaitRoot(root)
    return { file: await readFile(handle, path) }
  } catch (e) {
    return { error: errorMessage(e) }
  }
}

async function downloadWorkspacePayload(
  root: Promise<FileSystemDirectoryHandle>,
): Promise<{ file?: File; error?: string }> {
  try {
    const handle = await awaitRoot(root)
    const zip = await zipWorkspace(handle)
    const blob = await zip.generateAsync({ type: 'blob' })
    // The OPFS root has an empty name.
    const name = `${handle.name || 'workspace'}.zip`
    return { file: new File([blob], name, { type: 'application/zip' }) }
  } catch (e) {
    return { error: errorMessage(e) }
  }
}

// Replies to an AI Chat page request to download the workspace as a zip.
async function handleDownloadRequest(
  root: Promise<FileSystemDirectoryHandle>,
  event: MessageEvent,
) {
  const data = event.data as Record<string, unknown> | null
  if (
    typeof data !== 'object'
    || data === null
    || data.type !== kDownloadWorkspaceRequest
  ) {
    console.debug('[leo-workspace] ignoring non-DOWNLOAD_WORKSPACE message')
    return
  }
  const source = event.source as Window | null
  if (!source) {
    console.error(
      '[leo-workspace] DOWNLOAD_WORKSPACE has no source to reply to',
    )
    return
  }
  const response: DownloadWorkspaceResponse = {
    type: kDownloadWorkspaceResponse,
    requestId: data.requestId,
    ...(await downloadWorkspacePayload(root)),
  }
  source.postMessage(response, kAIChatOrigin)
}

// |root| resolves when the directory handle arrives via launchQueue, so a
// request that races ahead of it is served once it lands.
export function installMessageHandler(
  root: Promise<FileSystemDirectoryHandle>,
  origin = window.location.origin,
) {
  const targetOrigin = viewOrigin(origin)
  if (!targetOrigin) {
    console.error('[leo-workspace] no viewer origin to accept messages from')
    return
  }
  console.debug(
    '[leo-workspace] message handler installed, accepting from',
    targetOrigin,
  )

  window.addEventListener('message', async (event: MessageEvent) => {
    console.debug(
      '[leo-workspace] received message from',
      event.origin,
      event.data,
    )
    if (event.origin === kAIChatOrigin) {
      await handleDownloadRequest(root, event)
      return
    }
    if (event.origin !== targetOrigin) {
      console.debug(
        '[leo-workspace] ignoring message from wrong origin, expected',
        targetOrigin,
      )
      return
    }
    const data = event.data as Record<string, unknown> | null
    if (
      typeof data !== 'object'
      || data === null
      || data.type !== kReadFileRequest
    ) {
      console.debug('[leo-workspace] ignoring non-READ_FILE message')
      return
    }
    // Reply to the sender with an explicit target origin, never '*'.
    const source = event.source as Window | null
    if (!source) {
      console.error('[leo-workspace] READ_FILE has no source to reply to')
      return
    }
    console.debug('[leo-workspace] processing READ_FILE for path:', data.path)
    const response: ReadFileResponse = {
      type: kReadFileResponse,
      requestId: data.requestId,
      ...(await readFilePayload(root, data.path)),
    }
    console.debug(
      '[leo-workspace] sending response:',
      response.error || 'file loaded',
    )
    source.postMessage(response, targetOrigin)
  })
}
