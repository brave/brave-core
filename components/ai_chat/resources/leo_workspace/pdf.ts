// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Prints workspace files to PDF. The page can't do that itself, so the browser
// prints for it (LeoWorkspaceHandler.PrintFileToPdf), but the file is rendered
// exactly as it is for display: the workspace frames its `view.` sibling
// origin, which frames the file itself, served from the workspace folder by
// the viewer's service worker (see leo_workspace_view.tsx). That keeps
// relative stylesheets, images and fonts working, and keeps the file's content
// out of the workspace origin, which holds the folder handle:
//
//   workspace   chrome-untrusted://<uuid>.leo-workspace/
//   └ viewer    chrome-untrusted://view.<uuid>.leo-workspace/#file=<path>
//     └ file    chrome-untrusted://view.<uuid>.leo-workspace/<path>
//
// The viewer frame is given a unique name, which is how the browser finds the
// frame to print.

import {
  LeoWorkspaceHandler,
  LeoWorkspaceHandlerRemote,
} from 'gen/brave/components/ai_chat/core/common/mojom/leo_workspace.mojom.m.js'
import type { BigBuffer } from 'gen/mojo/public/mojom/base/big_buffer.mojom.m.js'
import {
  kFileDisplayed,
  kFileDisplayFailed,
  viewOrigin,
} from './message_handler'

// The Mojo JS API that WebUI bindings are given, which isn't in this bundle's
// type declarations.
declare const Mojo: { RESULT_OK: number }

// How long to wait for the viewer to display the file before giving up.
const kDisplayTimeoutMs = 30000

// The viewport the file is laid out in before printing, for any layout that
// depends on it rather than on the printed page.
const kViewportWidthPx = 1280
const kViewportHeightPx = 800

export interface PrintToPdfOptions {
  landscape: boolean
}

/** Renders the workspace file at |path| and returns it as a PDF. */
export type PrintFileToPdf = (
  path: string,
  options: PrintToPdfOptions,
) => Promise<Uint8Array<ArrayBuffer>>

let handler: LeoWorkspaceHandlerRemote | undefined
function getHandler(): LeoWorkspaceHandlerRemote {
  handler ??= LeoWorkspaceHandler.getRemote()
  return handler
}

// BigBuffer is a mojo union: either inlined bytes or shared memory to map.
export function readBigBuffer(buffer: BigBuffer): Uint8Array<ArrayBuffer> {
  if (buffer.bytes) {
    return new Uint8Array(buffer.bytes)
  }
  if (buffer.sharedMemory) {
    const { buffer: mapped, result } =
      buffer.sharedMemory.bufferHandle.mapBuffer(0, buffer.sharedMemory.size)
    if (result !== Mojo.RESULT_OK) {
      throw new Error('could not map the PDF')
    }
    // Copied, so the PDF outlives the mapping.
    return new Uint8Array(mapped).slice()
  }
  throw new Error('the browser returned no PDF')
}

// Displays |path| in a viewer frame that is laid out but out of sight, and
// resolves once the viewer reports that the file has loaded.
async function displayFile(
  path: string,
  frameName: string,
  origin: string,
): Promise<HTMLIFrameElement> {
  const viewerOrigin = viewOrigin(origin)
  if (!viewerOrigin) {
    throw new Error('the workspace has no viewer to render files with')
  }

  const iframe = document.createElement('iframe')
  iframe.name = frameName
  iframe.setAttribute('aria-hidden', 'true')
  iframe.tabIndex = -1
  Object.assign(iframe.style, {
    position: 'fixed',
    left: `-${kViewportWidthPx * 2}px`,
    top: '0',
    width: `${kViewportWidthPx}px`,
    height: `${kViewportHeightPx}px`,
    border: 'none',
  })

  const displayed = new Promise<void>((resolve, reject) => {
    const timeout = setTimeout(() => {
      done()
      reject(new Error(`timed out rendering ${path}`))
    }, kDisplayTimeoutMs)
    const onMessage = (event: MessageEvent) => {
      // Only this frame's viewer speaks for this file; the frame name could
      // be reused by any document.
      if (
        event.origin !== viewerOrigin
        || event.source !== iframe.contentWindow
      ) {
        return
      }
      const data = event.data as {
        type?: unknown
        file?: unknown
        error?: unknown
      } | null
      if (data?.file !== path) {
        return
      }
      if (data.type === kFileDisplayed) {
        done()
        resolve()
      } else if (data.type === kFileDisplayFailed) {
        done()
        reject(new Error(`could not render ${path}: ${String(data.error)}`))
      }
    }
    const done = () => {
      clearTimeout(timeout)
      window.removeEventListener('message', onMessage)
    }
    window.addEventListener('message', onMessage)
  })

  iframe.src = `${viewerOrigin}/#file=${encodeURIComponent(path)}`
  document.body.appendChild(iframe)
  try {
    await displayed
  } catch (e) {
    iframe.remove()
    throw e
  }
  return iframe
}

/**
 * Prints workspace files to PDF via the browser. |origin| is the workspace's
 * own origin, which its viewer origin is derived from.
 */
export function createPdfPrinter(
  origin = window.location.origin,
  getRemote: () => Pick<
    LeoWorkspaceHandlerRemote,
    'printFileToPdf'
  > = getHandler,
): PrintFileToPdf {
  return async (path, { landscape }) => {
    const frameName = `leo-workspace-pdf-${crypto.randomUUID()}`
    const iframe = await displayFile(path, frameName, origin)
    try {
      const { pdf, error } = await getRemote().printFileToPdf(
        frameName,
        landscape,
      )
      if (!pdf) {
        throw new Error(`could not print ${path} to PDF: ${error ?? 'unknown'}`)
      }
      return readBigBuffer(pdf)
    } finally {
      iframe.remove()
    }
  }
}
