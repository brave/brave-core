// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Minimal snap executor running inside chrome-untrusted://snap-executor/.
// It receives snap source code from the parent wallet page via postMessage
// and evaluates it in a lightweight sandbox. No runtime messaging is
// exposed to the loaded snap.

interface ExecuteSnapMessage {
  snapId: string
  sourceCode: string
  endowments: string[]
}

const PARENT_ORIGIN = 'chrome://wallet'

function sendToParent(message: unknown) {
  window.parent.postMessage(message, PARENT_ORIGIN)
}

function handleExecuteSnap(requestId: number, payload: ExecuteSnapMessage) {
  // Snap bundles are CommonJS, so provide `module`/`exports` and prefer an
  // explicit return value if the bundle produces one.
  const mod: { exports: Record<string, unknown> } = { exports: {} }
  try {
    const factory = new Function(
      'module',
      'exports',
      `"use strict";\n${payload.sourceCode}\n;return module.exports;`,
    )
    factory(mod, mod.exports)
    sendToParent({
      type: 'executeSnapResult',
      requestId,
      success: true,
      error: null,
    })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({
      type: 'executeSnapResult',
      requestId,
      success: false,
      error,
    })
  }
}

window.addEventListener('message', (event) => {
  if (event.origin !== PARENT_ORIGIN) {
    return
  }
  const command = event.data as {
    type: string
    requestId: number
    payload: ExecuteSnapMessage
  }
  if (!command || typeof command !== 'object') {
    return
  }

  if (command.type === 'executeSnap') {
    handleExecuteSnap(command.requestId, command.payload)
  }
})

// Notify the parent that the executor is ready.
sendToParent({ type: 'executorReady' })
