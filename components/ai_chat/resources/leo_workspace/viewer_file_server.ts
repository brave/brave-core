// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Serving a workspace file out of the folder the browser handed the viewer
// origin: what the service worker (sw.ts) answers a /files/<path> request with.
// Kept apart from the worker itself so that it can be exercised against a
// folder without one.

import { readFile } from './file_ops'
import { type FolderStore, filePathFromURL } from './viewer_files'

const kOctetStream = 'application/octet-stream'

// What a file is served with. The content is arbitrary workspace data - a file a
// model may have written - and it is navigated to, so it is served as a document
// that can show itself and nothing more:
//
// - `sandbox` keeps it from opening windows, submitting forms or navigating
//   anything but itself, which are the ways a document can carry bytes out;
// - `allow-same-origin` keeps it a client of the worker, so that the files it
//   goes on to ask for - its stylesheet, its images - are served too. Without it
//   the document gets an opaque origin, which no worker controls;
// - `allow-scripts` lets a page behave like a page. It is not a way out: script
//   here cannot register a worker (chrome-untrusted forbids it), cannot reach
//   WebMCP (blink allows it only for workspace hosts, not viewer ones), and has
//   no origin to load from but this one;
// - the source list allows nothing remote, so there is nothing to exfiltrate to,
//   and 'unsafe-inline' covers the inline script and style that ordinary
//   documents are written with;
// - nosniff keeps the file the type it was served as.
const kFileResponseCSP = [
  'sandbox allow-same-origin allow-scripts',
  "default-src 'self'",
  "script-src 'self' 'unsafe-inline'",
  "style-src 'self' 'unsafe-inline'",
  "img-src 'self' data: blob:",
  "font-src 'self' data:",
  "media-src 'self'",
  "frame-src 'self'",
  "object-src 'none'",
  "base-uri 'none'",
  "form-action 'none'",
].join('; ')

export const kFileResponseHeaders = {
  'content-security-policy': kFileResponseCSP,
  'x-content-type-options': 'nosniff',
}

// What the worker turns into a Response. A plain descriptor rather than a
// Response so that tests need no fetch implementation.
export interface FileResponse {
  status: number
  contentType: string
  body: Blob | string
}

// The folder is read once and then kept, because a request for a document is
// followed by requests for whatever it references.
let folder: Promise<FileSystemDirectoryHandle | null> | null = null

export function forgetFolderForTesting() {
  folder = null
}

function statusForError(error: unknown): number {
  const name = error instanceof DOMException ? error.name : ''
  if (name === 'NotFoundError' || name === 'TypeMismatchError') {
    return 404
  }
  if (name === 'NotAllowedError') {
    return 403
  }
  // A path that leaves the folder is refused by file_ops before the platform
  // sees it, as is a name the platform rejects.
  return error instanceof TypeError || error instanceof Error ? 404 : 500
}

// Serves the file |url| asks for out of |store|'s folder.
export async function serveFile(
  url: URL,
  store: FolderStore,
): Promise<FileResponse | null> {
  const path = filePathFromURL(url)
  if (path === null) {
    return null
  }

  folder ??= store.load()
  const root = await folder
  if (!root) {
    // The viewer page puts the folder in storage before it asks for a file, so
    // this is a request that arrived without one having been shown.
    folder = null
    return {
      status: 503,
      contentType: 'text/plain',
      body: 'no workspace folder to read from',
    }
  }

  try {
    const file = await readFile(root, path)
    return {
      status: 200,
      contentType: file.type || kOctetStream,
      body: file,
    }
  } catch (error) {
    return {
      status: statusForError(error),
      contentType: 'text/plain',
      body: `could not read ${path}: ${
        error instanceof Error ? error.message : String(error)
      }`,
    }
  }
}
