// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// postMessage bridge that lets the `view.` sibling origin read files out of the
// workspace folder. Only that origin is accepted: the workspace holds an
// auto-granted File System Access handle to a user-picked folder.

import { readFile } from './file_ops'

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

async function readFilePayload(
  root: Promise<FileSystemDirectoryHandle>,
  path: unknown,
): Promise<{ file?: File; error?: string }> {
  // Checked before awaiting the handle so a malformed request fails fast.
  if (typeof path !== 'string' || path === '') {
    return { error: 'READ_FILE requires a non-empty string "path"' }
  }
  try {
    return { file: await readFile(await root, path) }
  } catch (e) {
    return { error: e instanceof Error ? e.message : String(e) }
  }
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

  window.addEventListener('message', async (event: MessageEvent) => {
    if (event.origin !== targetOrigin) {
      return
    }
    const data = event.data as Record<string, unknown> | null
    if (
      typeof data !== 'object'
      || data === null
      || data.type !== kReadFileRequest
    ) {
      return
    }
    // Reply to the sender with an explicit target origin, never '*'.
    const source = event.source as Window | null
    if (!source) {
      console.error('[leo-workspace] READ_FILE has no source to reply to')
      return
    }
    const response: ReadFileResponse = {
      type: kReadFileResponse,
      requestId: data.requestId,
      ...(await readFilePayload(root, data.path)),
    }
    source.postMessage(response, targetOrigin)
  })
}
