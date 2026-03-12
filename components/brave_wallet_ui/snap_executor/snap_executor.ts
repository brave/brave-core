// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// snap_executor.ts runs inside a chrome-untrusted://snap-executor/ iframe.
// One instance is created per snap. SES lockdown() has already been called
// via ses_lockdown.bundle.js before this script executes, so Compartment
// and harden are available as globals.

// SES globals injected by lockdown() — not in standard TypeScript types.
declare const Compartment: new (globals: Record<string, unknown>) => {
  evaluate(src: string): unknown
}
declare function harden<T>(value: T): T

const INVOKE_TIMEOUT_MS = 60_000

// The snap's onRpcRequest handler, set after load_snap succeeds.
let snapOnRpcRequest:
  | ((args: {
      origin: string
      request: { method: string; params: unknown }
    }) => Promise<unknown>)
  | null = null

// Pending snap.request() calls waiting for a response from the parent.
const pendingSnapRequests = new Map<
  string,
  { resolve: (value: unknown) => void; reject: (reason: Error) => void }
>()

function generateRequestId(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2)}`
}

function sendToParent(message: Record<string, unknown>): void {
  window.parent.postMessage(message, '*')
}

function sendResponse(
  requestId: string,
  result?: unknown,
  error?: string,
): void {
  sendToParent({ type: 'response', requestId, result, error })
}

async function handleLoadSnap(
  snapId: string,
  source: string,
  requestId: string,
): Promise<void> {
  try {
    // snap.request() — called by snap code to invoke core APIs (e.g. key derivation).
    // Sends a snap_request message to the wallet page, which relays it to C++ core,
    // then resolves/rejects when the response arrives via snap_request_response.
    const snapRequestFn = (params: {
      method: string
      params?: unknown
    }): Promise<unknown> => {
      return new Promise((resolve, reject) => {
        const snapReqId = generateRequestId()
        pendingSnapRequests.set(snapReqId, { resolve, reject })
        sendToParent({
          type: 'snap_request',
          snapId,
          method: params.method,
          params: params.params,
          requestId: snapReqId,
        })
      })
    }

    // Expose snap exports so the Compartment can capture them.
    // CommonJS-style snaps set module.exports.onRpcRequest.
    const snapExports: Record<string, unknown> = {}
    const snapModule = { exports: snapExports }

    const compartment = new Compartment({
      module: harden(snapModule),
      exports: harden(snapExports),
      snap: harden({ request: snapRequestFn }),
      console: harden({
        log: console.log.bind(console),
        warn: console.warn.bind(console),
        error: console.error.bind(console),
        info: console.info.bind(console),
        debug: console.debug.bind(console),
      }),
      TextEncoder: harden(TextEncoder),
      TextDecoder: harden(TextDecoder),
    })

    compartment.evaluate(source)

    const handler = snapExports['onRpcRequest']
    if (typeof handler !== 'function') {
      sendResponse(requestId, undefined, 'Snap does not export onRpcRequest')
      return
    }

    snapOnRpcRequest = handler as typeof snapOnRpcRequest
    sendResponse(requestId, true)
  } catch (err) {
    const msg = err instanceof Error ? err.message : String(err)
    sendResponse(requestId, undefined, `Failed to load snap: ${msg}`)
  }
}

async function handleInvoke(
  method: string,
  params: unknown,
  origin: string,
  requestId: string,
): Promise<void> {
  if (typeof snapOnRpcRequest !== 'function') {
    sendResponse(requestId, undefined, 'Snap not loaded')
    return
  }

  let settled = false

  const timeoutId = setTimeout(() => {
    if (!settled) {
      settled = true
      sendResponse(
        requestId,
        undefined,
        `Snap RPC timed out after ${INVOKE_TIMEOUT_MS}ms`,
      )
    }
  }, INVOKE_TIMEOUT_MS)

  try {
    const result = await snapOnRpcRequest({
      origin,
      request: { method, params },
    })
    clearTimeout(timeoutId)
    if (!settled) {
      settled = true
      sendResponse(requestId, result)
    }
  } catch (err) {
    clearTimeout(timeoutId)
    if (!settled) {
      settled = true
      const msg = err instanceof Error ? err.message : String(err)
      sendResponse(requestId, undefined, msg)
    }
  }
}

function handleSnapRequestResponse(
  snapReqId: string,
  result: unknown,
  error?: string,
): void {
  const pending = pendingSnapRequests.get(snapReqId)
  if (!pending) {
    return
  }
  pendingSnapRequests.delete(snapReqId)
  if (error) {
    pending.reject(new Error(error))
  } else {
    pending.resolve(result)
  }
}

window.addEventListener('message', (event: MessageEvent) => {
  // Only accept messages from the parent wallet page.
  if (event.source !== window.parent) {
    return
  }

  const data = event.data as Record<string, unknown>
  if (!data || typeof data !== 'object' || typeof data['type'] !== 'string') {
    return
  }

  switch (data['type']) {
    case 'load_snap':
      handleLoadSnap(
        data['snapId'] as string,
        data['source'] as string,
        data['requestId'] as string,
      )
      break

    case 'invoke':
      handleInvoke(
        data['method'] as string,
        data['params'],
        (data['origin'] as string) ?? '',
        data['requestId'] as string,
      )
      break

    case 'snap_request_response':
      // Core (C++) responded to a snap.request() call relayed by the bridge.
      handleSnapRequestResponse(
        data['requestId'] as string,
        data['result'],
        data['error'] as string | undefined,
      )
      break
  }
})
