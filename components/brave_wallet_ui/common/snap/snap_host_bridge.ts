// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// SnapHostBridge for the wallet page. Embeds snap iframes at
// chrome-untrusted://snap-host/ and loads snap source into them via
// postMessage. Mojo (mojom::SnapHostBridge) stays on this trusted page; the
// untrusted frame has no Mojo pipe.

import { BraveWallet } from '../../constants/types'
import {
  ExecuteSnapResult,
  isExecuteSnapResult,
  isExecutorReady,
  SNAP_HOST_ORIGIN,
  SnapCommand,
} from './snap_messages'

// The host's HTML is served via SetDefaultResource at the origin root.
const SNAP_HOST_URL = `${SNAP_HOST_ORIGIN}/`

const HANDSHAKE_TIMEOUT_MS = 30000
const COMMAND_TIMEOUT_MS = 60000

interface SnapConnection {
  iframe: HTMLIFrameElement
  ready: boolean
}

export class SnapHostBridge {
  private readonly connections = new Map<string, SnapConnection>()
  private readonly pendingConnections = new Map<
    string,
    Promise<SnapConnection>
  >()
  private readonly container: HTMLElement
  private nextCommandId = 0
  // Held so the receiver stays reachable for the lifetime of the bridge and
  // can be closed explicitly.
  private receiver?: BraveWallet.SnapHostBridgeReceiver

  constructor(container?: HTMLElement) {
    this.container = container ?? document.body
  }

  // Creates the SnapHostBridge pipe and returns the remote end for the
  // browser. The receiver is retained so close() can drop it.
  bind(): BraveWallet.SnapHostBridgeRemote {
    this.receiver = new BraveWallet.SnapHostBridgeReceiver(this)
    return this.receiver.$.bindNewPipeAndPassRemote()
  }

  close() {
    this.receiver?.$.close()
    this.receiver = undefined
    for (const snapId of [...this.connections.keys()]) {
      this.unloadSnap(snapId)
    }
  }

  // ---------------------------------------------------------------------------
  // SnapHostBridge Mojo interface — called by the browser bridge controller
  // ---------------------------------------------------------------------------

  async loadSnap(
    snapId: string,
    sourceCode: string,
  ): Promise<{
    success: boolean
    error: string | null
    result: string | null
  }> {
    try {
      let conn = this.connections.get(snapId)
      if (!conn) {
        conn = await this.createConnection(snapId)
      }

      const executeResult = await this.sendCommand(conn, {
        type: SnapCommand.ExecuteSnap,
        payload: { snapId, sourceCode },
      })

      if (!executeResult.success) {
        this.unloadSnap(snapId)
      }

      return {
        success: executeResult.success === true,
        error: executeResult.error ?? null,
        result:
          typeof executeResult.result === 'string'
            ? executeResult.result
            : null,
      }
    } catch (err) {
      this.unloadSnap(snapId)
      const msg = err instanceof Error ? err.message : String(err)
      return { success: false, error: msg, result: null }
    }
  }

  unloadSnap(snapId: string): void {
    const conn = this.connections.get(snapId)
    if (conn) {
      conn.iframe.remove()
      this.connections.delete(snapId)
    }
    this.pendingConnections.delete(snapId)
  }

  // ---------------------------------------------------------------------------
  // Private helpers
  // ---------------------------------------------------------------------------

  private createConnection(snapId: string): Promise<SnapConnection> {
    const pending = this.pendingConnections.get(snapId)
    if (pending) {
      return pending
    }

    const promise = new Promise<SnapConnection>((resolve, reject) => {
      const iframe = document.createElement('iframe')
      // The schemes keep the parent and frame cross-origin. allow-same-origin
      // gives the frame a stable snap-host origin; without it, postMessage
      // arrives with origin "null", failing every origin check below.
      iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
      iframe.style.display = 'none'
      iframe.src = SNAP_HOST_URL

      const cleanup = () => {
        window.removeEventListener('message', onMessage)
        window.clearTimeout(timer)
        this.pendingConnections.delete(snapId)
      }

      const fail = (error: Error) => {
        cleanup()
        iframe.remove()
        reject(error)
      }

      const onMessage = (event: MessageEvent) => {
        if (
          event.origin !== SNAP_HOST_ORIGIN
          || event.source !== iframe.contentWindow
        ) {
          return
        }
        if (isExecutorReady(event.data)) {
          cleanup()
          const conn: SnapConnection = { iframe, ready: true }
          this.connections.set(snapId, conn)
          resolve(conn)
        }
      }

      const timer = window.setTimeout(() => {
        fail(new Error('Snap host handshake timed out'))
      }, HANDSHAKE_TIMEOUT_MS)

      window.addEventListener('message', onMessage)
      this.container.appendChild(iframe)
    })

    this.pendingConnections.set(snapId, promise)
    return promise
  }

  private sendCommand(
    conn: SnapConnection,
    command: {
      type: SnapCommand.ExecuteSnap
      payload: { snapId: string; sourceCode: string }
    },
  ): Promise<ExecuteSnapResult> {
    return new Promise((resolve, reject) => {
      if (!conn.ready || !conn.iframe.contentWindow) {
        reject(new Error('Snap connection not ready'))
        return
      }

      const requestId = ++this.nextCommandId
      let settled = false

      const settle = (fn: () => void) => {
        if (settled) {
          return
        }
        settled = true
        window.removeEventListener('message', handler)
        window.clearTimeout(timer)
        fn()
      }

      const handler = (event: MessageEvent) => {
        if (
          event.origin !== SNAP_HOST_ORIGIN
          || event.source !== conn.iframe.contentWindow
        ) {
          return
        }
        if (
          isExecuteSnapResult(event.data)
          && event.data.requestId === requestId
        ) {
          settle(() => resolve(event.data))
        }
      }

      // `settle` closes over `timer`, so it must be assigned before anything
      // that can invoke the handler.
      const timer = window.setTimeout(() => {
        settle(() =>
          reject(new Error(`Snap command '${command.type}' timed out`)),
        )
      }, COMMAND_TIMEOUT_MS)

      window.addEventListener('message', handler)
      // TODO(https://github.com/brave/brave-browser/issues/58686): replace
      // postMessage with a Mojo pipe into the untrusted frame.
      conn.iframe.contentWindow.postMessage(
        {
          type: command.type,
          requestId,
          payload: command.payload,
        },
        SNAP_HOST_ORIGIN,
      )
    })
  }
}
