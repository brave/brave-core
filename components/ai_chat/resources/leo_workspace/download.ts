// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// postMessage protocol that lets the AI Chat page (which frames the workspace
// in its file preview lightbox) ask the workspace for a zip of all its files.
// The workspace side lives in message_handler.ts. This module is imported by
// the AI Chat page, so it must stay free of the workspace's file code.

export const kDownloadWorkspaceRequest = 'DOWNLOAD_WORKSPACE'
export const kDownloadWorkspaceResponse = 'DOWNLOAD_WORKSPACE_RESULT'

// The only origin the workspace accepts download requests from.
export const kAIChatOrigin = 'chrome://leo-ai'

export interface DownloadWorkspaceResponse {
  type: typeof kDownloadWorkspaceResponse
  requestId?: unknown
  // The zip archive, named after the workspace folder.
  file?: File
  error?: string
}

// Asks the workspace frame |target| (whose origin must be |targetOrigin|) for a
// zip of every file in it. Rejects if the workspace reports an error or
// |signal| is aborted.
export function requestWorkspaceZip(
  target: Window,
  targetOrigin: string,
  signal?: AbortSignal,
): Promise<File> {
  return new Promise((resolve, reject) => {
    const requestId = crypto.randomUUID()

    const cleanup = () => {
      window.removeEventListener('message', onMessage)
      signal?.removeEventListener('abort', onAbort)
    }

    const onAbort = () => {
      cleanup()
      reject(signal?.reason ?? new Error('aborted'))
    }

    const onMessage = (event: MessageEvent) => {
      if (event.origin !== targetOrigin || event.source !== target) {
        return
      }
      const data = event.data as DownloadWorkspaceResponse | null
      if (
        typeof data !== 'object'
        || data === null
        || data.type !== kDownloadWorkspaceResponse
        || data.requestId !== requestId
      ) {
        return
      }
      cleanup()
      if (data.file) {
        resolve(data.file)
      } else {
        reject(new Error(data.error || 'No archive in response'))
      }
    }

    if (signal?.aborted) {
      onAbort()
      return
    }
    signal?.addEventListener('abort', onAbort)
    window.addEventListener('message', onMessage)
    target.postMessage(
      { type: kDownloadWorkspaceRequest, requestId },
      targetOrigin,
    )
  })
}
