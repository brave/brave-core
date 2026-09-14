// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Minimal SnapBridge implementation for the wallet page.
// It embeds snap iframes pointing at chrome-untrusted://snap-executor/ and
// loads snap source into them. This extraction omits MetaMask's
// post-message-stream / object-multiplex integration and SES lockdown.

import { BraveWallet } from '../../constants/types'

const SNAP_EXECUTOR_ORIGIN = 'chrome-untrusted://snap-executor'
// The executor's HTML is served via SetDefaultResource at the root path only
// -- kSnapExecutorGenerated maps snap_executor.bundle.js, not
// snap_executor.html -- so the iframe must load the origin root.
const SNAP_EXECUTOR_URL = `${SNAP_EXECUTOR_ORIGIN}/`

interface SnapConnection {
  iframe: HTMLIFrameElement
  ready: boolean
}

export class SnapBridge {
  private readonly connections = new Map<string, SnapConnection>()
  private snapsService: BraveWallet.SnapsServiceRemote | null = null
  private readonly container: HTMLElement
  private nextCommandId = 0

  constructor(container?: HTMLElement) {
    this.container = container ?? document.body
  }

  setSnapsService(svc: BraveWallet.SnapsServiceRemote): void {
    this.snapsService = svc
  }

  bindNewPipeAndPassRemote() {
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
      // Cross-scheme chrome:// -> chrome-untrusted:// keeps isolation; without
      // allow-same-origin the frame gets an opaque origin and its postMessage
      // arrives with origin "null", failing every origin check below.
      iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
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

      const requestId = ++this.nextCommandId

      const handler = (event: MessageEvent) => {
        if (event.origin !== SNAP_EXECUTOR_ORIGIN) {
          return
        }
        const data = event.data as { type: string; requestId: number }
        if (data?.type === `${type}Result` && data.requestId === requestId) {
          window.removeEventListener('message', handler)
          window.clearTimeout(timer)
          resolve(data)
        }
      }

      window.addEventListener('message', handler)
      conn.iframe.contentWindow.postMessage(
        { type, requestId, payload },
        SNAP_EXECUTOR_ORIGIN,
      )

      // Timeout to avoid leaking the listener.
      const timer = window.setTimeout(() => {
        window.removeEventListener('message', handler)
        reject(new Error(`Snap command '${type}' timed out`))
      }, 60000)
    })
  }
}
