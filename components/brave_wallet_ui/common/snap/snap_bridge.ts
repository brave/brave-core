// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Minimal SnapBridge implementation for the wallet page.
// It manages snap iframes pointing at chrome-untrusted://snap-executor/ and
// communicates with them via simple postMessage. This extraction omits
// MetaMask's post-message-stream / object-multiplex integration and SES
// lockdown.

import { BraveWallet } from '../../constants/types'

const SNAP_EXECUTOR_ORIGIN = 'chrome-untrusted://snap-executor'
const SNAP_EXECUTOR_URL = `${SNAP_EXECUTOR_ORIGIN}/snap_executor.html`

interface SnapConnection {
  iframe: HTMLIFrameElement
  ready: boolean
}

// Convert a mojo_base.mojom.Value tagged union to a plain JS value.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export function mojoValueToJs(v: any): unknown {
  if (v === null || v === undefined) {
    return null
  }
  if (v.nullValue !== undefined) {
    return null
  }
  if (v.boolValue !== undefined) {
    return v.boolValue
  }
  if (v.intValue !== undefined) {
    return v.intValue
  }
  if (v.doubleValue !== undefined) {
    return v.doubleValue
  }
  if (v.stringValue !== undefined) {
    return v.stringValue
  }
  if (v.listValue !== undefined) {
    return v.listValue.storage.map((item: any) => mojoValueToJs(item))
  }
  if (v.dictionaryValue !== undefined) {
    const obj: Record<string, unknown> = {}
    for (const [k, val] of Object.entries(v.dictionaryValue.storage)) {
      obj[k] = mojoValueToJs(val)
    }
    return obj
  }
  return null
}

// Convert a plain JS value to a mojo_base.mojom.Value tagged union.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export function jsToMojoValue(v: unknown): any {
  if (v === null || v === undefined) {
    return { nullValue: 0 }
  }
  if (typeof v === 'boolean') {
    return { boolValue: v }
  }
  if (typeof v === 'number') {
    return Number.isInteger(v) ? { intValue: v } : { doubleValue: v }
  }
  if (typeof v === 'string') {
    return { stringValue: v }
  }
  if (Array.isArray(v)) {
    return { listValue: { storage: v.map((item) => jsToMojoValue(item)) } }
  }
  if (typeof v === 'object') {
    const storage: Record<string, unknown> = {}
    for (const [k, val] of Object.entries(v as Record<string, unknown>)) {
      storage[k] = jsToMojoValue(val)
    }
    return { dictionaryValue: { storage } }
  }
  return { nullValue: 0 }
}

export class SnapBridge {
  private readonly connections = new Map<string, SnapConnection>()
  private snapRequestHandler: BraveWallet.SnapRequestHandlerRemote | null = null
  private snapsService: BraveWallet.SnapsServiceRemote | null = null
  private readonly container: HTMLElement

  constructor(container?: HTMLElement) {
    this.container = container ?? document.body
  }

  setSnapRequestHandler(handler: BraveWallet.SnapRequestHandlerRemote): void {
    this.snapRequestHandler = handler
  }

  setSnapsService(svc: BraveWallet.SnapsServiceRemote): void {
    this.snapsService = svc
  }

  bindNewPipeAndPassRemote() {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const receiver = new BraveWallet.SnapBridgeReceiver(this as any)
    return receiver.$.bindNewPipeAndPassRemote()
  }

  // ---------------------------------------------------------------------------
  // SnapBridge Mojo interface — called by C++ SnapsService
  // ---------------------------------------------------------------------------

  async loadSnap(
    snapId: string,
  ): Promise<{ success: boolean; error: string | null }> {
    try {
      let conn = this.connections.get(snapId)
      if (!conn) {
        conn = await this.createConnection(snapId)
      }

      if (!this.snapsService) {
        return { success: false, error: 'SnapsService not available' }
      }
      const { sourceCode: code, error } =
        await this.snapsService.getSnapBundle(snapId)
      if (error || !code) {
        return { success: false, error: error ?? 'Bundle not found' }
      }

      const result = await this.sendCommand(conn, 'executeSnap', {
        snapId,
        sourceCode: code,
        endowments: [],
      })

      return { success: result.success === true, error: result.error ?? null }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      return { success: false, error: msg }
    }
  }

  async invokeSnap(
    snapId: string,
    method: string,
    params: unknown,
    callerOrigin: string,
  ): Promise<{ result: unknown | null; error: string | null }> {
    try {
      const conn = this.connections.get(snapId)
      if (!conn) {
        return { result: null, error: `Snap '${snapId}' is not loaded` }
      }

      const plainParams = mojoValueToJs(params)

      const response = await this.sendCommand(conn, 'snapRpc', {
        snapId,
        handler: 'onRpcRequest',
        origin: callerOrigin,
        request: {
          jsonrpc: '2.0',
          method,
          params: plainParams,
        },
      })

      const mojoResult =
        response.result !== undefined && response.result !== null
          ? jsToMojoValue(response.result)
          : null
      return { result: mojoResult, error: response.error ?? null }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      return { result: null, error: msg }
    }
  }

  unloadSnap(snapId: string): void {
    const conn = this.connections.get(snapId)
    if (conn) {
      conn.iframe.remove()
      this.connections.delete(snapId)
    }
  }

  // ---------------------------------------------------------------------------
  // Private helpers
  // ---------------------------------------------------------------------------

  private createConnection(snapId: string): Promise<SnapConnection> {
    return new Promise((resolve, reject) => {
      const iframe = document.createElement('iframe')
      iframe.sandbox.add('allow-scripts')
      iframe.style.display = 'none'
      iframe.src = SNAP_EXECUTOR_URL

      const onMessage = (event: MessageEvent) => {
        if (event.origin !== SNAP_EXECUTOR_ORIGIN) {
          return
        }
        const data = event.data as { type: string }
        if (data?.type === 'executorReady') {
          window.removeEventListener('message', onMessage)
          const conn: SnapConnection = { iframe, ready: true }
          this.connections.set(snapId, conn)
          resolve(conn)
        }
      }

      window.addEventListener('message', onMessage)
      iframe.onerror = () => reject(new Error('Failed to load snap executor'))

      this.container.appendChild(iframe)
    })
  }

  private sendCommand(
    conn: SnapConnection,
    type: string,
    payload: unknown,
  ): Promise<any> {
    return new Promise((resolve, reject) => {
      if (!conn.ready || !conn.iframe.contentWindow) {
        reject(new Error('Snap connection not ready'))
        return
      }

      const handler = (event: MessageEvent) => {
        if (event.origin !== SNAP_EXECUTOR_ORIGIN) {
          return
        }
        const data = event.data as { type: string }
        if (data?.type === `${type}Result`) {
          window.removeEventListener('message', handler)
          resolve(data)
        }
      }

      window.addEventListener('message', handler)
      conn.iframe.contentWindow.postMessage(
        { type, payload },
        SNAP_EXECUTOR_ORIGIN,
      )

      // Timeout to avoid leaking the listener.
      window.setTimeout(() => {
        window.removeEventListener('message', handler)
        reject(new Error(`Snap command '${type}' timed out`))
      }, 60000)
    })
  }
}
