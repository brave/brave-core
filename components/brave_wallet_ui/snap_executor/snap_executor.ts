// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Minimal snap executor running inside chrome-untrusted://snap-executor/.
// It receives snap source code from the parent wallet page via postMessage,
// evaluates it in a lightweight sandbox, and exposes snap.request() to the
// running snap. This extraction intentionally omits SES lockdown, manifest
// parsing, and MetaMask's IFrameSnapExecutor.

interface SnapRequest {
  jsonrpc: '2.0'
  method: string
  params: unknown
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
  | { type: 'executeSnap'; payload: ExecuteSnapMessage }
  | { type: 'snapRpc'; payload: SnapRpcMessage }

const PARENT_ORIGIN = 'chrome://wallet'

let snapModule: { onRpcRequest?: (request: SnapRequest) => unknown } | null =
  null
let currentSnapId: string | null = null

function sendToParent(message: unknown) {
  window.parent.postMessage(message, PARENT_ORIGIN)
}

async function handleExecuteSnap(payload: ExecuteSnapMessage) {
  currentSnapId = payload.snapId

  // Build a minimal snap global with a snap.request() handle.
  // In this minimal executor snap.request() is a no-op that returns null.
  const snapGlobal = {
    request: async (_request: SnapRequest) => {
      // The real implementation would forward to the parent via postMessage and
      // wait for a response. For the minimal extraction we return null so the
      // snap can at least boot.
      return null
    },
  }

  // Wrap the source code so it receives the snap global.
  const wrapped = `
    (function(snap) {
      ${payload.sourceCode}
    })(snapGlobal)
  `

  try {
    // eslint-disable-next-line @typescript-eslint/no-implied-eval
    snapModule = eval(wrapped) as typeof snapModule
    if (typeof snapModule !== 'object' && typeof snapModule !== 'function') {
      snapModule = null
    }
    sendToParent({ type: 'executeSnapResult', success: true, error: null })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({ type: 'executeSnapResult', success: false, error })
  }
}

async function handleSnapRpc(payload: SnapRpcMessage) {
  if (!snapModule || typeof snapModule.onRpcRequest !== 'function') {
    sendToParent({
      type: 'snapRpcResult',
      error: 'Snap does not expose onRpcRequest',
    })
    return
  }

  try {
    const result = await snapModule.onRpcRequest(payload.request)
    sendToParent({ type: 'snapRpcResult', result, error: null })
  } catch (err) {
    const error = err instanceof Error ? err.message : String(err)
    sendToParent({ type: 'snapRpcResult', result: null, error })
  }
}

window.addEventListener('message', (event) => {
  if (event.origin !== PARENT_ORIGIN) {
    return
  }
  const message = event.data as ParentCommand
  if (!message || typeof message !== 'object') {
    return
  }

  if (message.type === 'executeSnap') {
    void handleExecuteSnap(message.payload)
  } else if (message.type === 'snapRpc') {
    void handleSnapRpc(message.payload)
  }
})

// Notify the parent that the executor is ready.
sendToParent({ type: 'executorReady' })
