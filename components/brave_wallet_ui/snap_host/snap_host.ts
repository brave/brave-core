// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Snap host running inside chrome-untrusted://snap-host/.
// Receives snap source from the parent wallet page via postMessage and
// evaluates it with new Function().

import {
  ALLOWED_PARENT_ORIGINS,
  ExecuteSnapPayload,
  isExecuteSnapCommand,
  SnapMessageType,
} from '../common/snap/snap_messages'

let parentOrigin: string | null = null

function sendToParent(message: unknown) {
  if (parentOrigin) {
    window.parent.postMessage(message, parentOrigin)
    return
  }
  // Pre-handshake: postMessage silently drops a mismatched targetOrigin, so
  // exactly one of these is delivered.
  for (const origin of ALLOWED_PARENT_ORIGINS) {
    window.parent.postMessage(message, origin)
  }
}

function handleExecuteSnap(requestId: number, payload: ExecuteSnapPayload) {
  // Snap bundles are CommonJS, so provide `module`/`exports` and prefer an
  // explicit return value if the bundle produces one.
  const mod: { exports: unknown } = { exports: {} }
  try {
    const factory = new Function(
      'module',
      'exports',
      `"use strict";\n${payload.sourceCode}\n;return module.exports;`,
    )
    const exported = factory(mod, mod.exports)
    sendToParent({
      type: SnapMessageType.ExecuteSnapResult,
      requestId,
      success: true,
      error: null,
      result: typeof exported === 'string' ? exported : null,
    })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({
      type: SnapMessageType.ExecuteSnapResult,
      requestId,
      success: false,
      error,
      result: null,
    })
  }
}

window.addEventListener('message', (event) => {
  // event.source must stay ANDed with the origin check, or a same-origin
  // sibling frame could win the origin-pinning race below.
  if (
    event.source !== window.parent
    || !ALLOWED_PARENT_ORIGINS.includes(event.origin)
  ) {
    return
  }
  if (!isExecuteSnapCommand(event.data)) {
    return
  }
  parentOrigin = event.origin
  handleExecuteSnap(event.data.requestId, event.data.payload)
})

// Notify the parent that the host is ready.
sendToParent({ type: SnapMessageType.ExecutorReady })
