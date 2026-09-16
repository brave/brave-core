// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Minimal snap executor running inside chrome-untrusted://snap-executor/.
// It receives snap source code from the parent wallet page via postMessage,
// evaluates it in a lightweight sandbox, and exposes snap.request() to the
// running snap. This extraction intentionally omits SES lockdown, manifest
// parsing, and MetaMask's IFrameSnapExecutor.

interface SnapRequest {
  jsonrpc: '2.0'
  method: string
  params?: unknown
}

interface SnapRpcMessage {
  snapId: string
  handler: 'onRpcRequest'
  origin: string
  request: SnapRequest
}

interface ExecuteSnapMessage {
  snapId: string
  sourceCode: string
  endowments: string[]
}

type ParentCommand =
  | { type: 'executeSnap'; requestId: number; payload: ExecuteSnapMessage }
  | { type: 'snapRpc'; requestId: number; payload: SnapRpcMessage }

const PARENT_ORIGIN = 'chrome://wallet'

let snapModule: { onRpcRequest?: (request: SnapRequest) => unknown } | null =
  null
let currentSnapId: string | null = null

let nextSnapRequestId = 0
const pendingSnapRequests = new Map<
  number,
  { resolve: (v: unknown) => void; reject: (e: Error) => void }
>()

function sendToParent(message: unknown) {
  window.parent.postMessage(message, PARENT_ORIGIN)
}

async function handleExecuteSnap(
  requestId: number,
  payload: ExecuteSnapMessage,
) {
  currentSnapId = payload.snapId

  const snapGlobal = {
    request: (request: SnapRequest) =>
      new Promise((resolve, reject) => {
        const snapRequestId = ++nextSnapRequestId
        pendingSnapRequests.set(snapRequestId, { resolve, reject })
        sendToParent({
          type: 'snapRequest',
          requestId: snapRequestId,
          snapId: currentSnapId,
          method: request.method,
          params: request.params ?? null,
        })
      }),
  }

  // Snap bundles are CommonJS, so provide `module`/`exports` and prefer an
  // explicit return value if the bundle produces one.
  const mod: { exports: Record<string, unknown> } = { exports: {} }
  try {
    const factory = new Function(
      'snap',
      'module',
      'exports',
      `"use strict";\n${payload.sourceCode}\n;return module.exports;`,
    )
    const returned = factory(snapGlobal, mod, mod.exports)
    snapModule =
      returned && typeof returned === 'object'
        ? (returned as typeof snapModule)
        : (mod.exports as typeof snapModule)
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

async function handleSnapRpc(requestId: number, payload: SnapRpcMessage) {
  if (!snapModule || typeof snapModule.onRpcRequest !== 'function') {
    sendToParent({
      type: 'snapRpcResult',
      requestId,
      result: null,
      error: 'Snap does not expose onRpcRequest',
    })
    return
  }

  try {
    const result = await snapModule.onRpcRequest(payload.request)
    sendToParent({ type: 'snapRpcResult', requestId, result, error: null })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({
      type: 'snapRpcResult',
      requestId,
      result: null,
      error,
    })
  }
}

window.addEventListener('message', (event) => {
  if (event.origin !== PARENT_ORIGIN) {
    return
  }
  const message = event.data as {
    type: string
    requestId: number
    result?: unknown
    error?: string | null
  }
  if (!message || typeof message !== 'object') {
    return
  }

  if (message.type === 'snapRequestResult') {
    const pending = pendingSnapRequests.get(message.requestId)
    if (pending) {
      pendingSnapRequests.delete(message.requestId)
      if (message.error) {
        pending.reject(new Error(message.error))
      } else {
        pending.resolve(message.result ?? null)
      }
    }
    return
  }

  const command = event.data as ParentCommand
  if (command.type === 'executeSnap') {
    void handleExecuteSnap(command.requestId, command.payload)
  } else if (command.type === 'snapRpc') {
    void handleSnapRpc(command.requestId, command.payload)
  }
})

// Notify the parent that the executor is ready.
sendToParent({ type: 'executorReady' })
