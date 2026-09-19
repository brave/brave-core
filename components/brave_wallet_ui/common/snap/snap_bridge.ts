// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// SnapBridge for the wallet page. Embeds snap iframes at
// chrome-untrusted://snap-executor/ and loads snap source into them via
// postMessage. Mojo (mojom::SnapBridge) stays on this trusted page; the
// untrusted frame has no Mojo pipe.

import { BraveWallet } from '../../constants/types'
import {
  ExecuteSnapResult,
  isExecuteSnapResult,
  isExecutorReady,
  SNAP_EXECUTOR_ORIGIN,
  SnapCommand,
} from './snap_messages'

// The executor's HTML is served via SetDefaultResource at the origin root.
const SNAP_EXECUTOR_URL = `${SNAP_EXECUTOR_ORIGIN}/`

const HANDSHAKE_TIMEOUT_MS = 30000
const COMMAND_TIMEOUT_MS = 60000

interface SnapConnection {
  iframe: HTMLIFrameElement
  ready: boolean
}

export class SnapBridge {
  private readonly connections = new Map<string, SnapConnection>()
  private readonly pendingConnections = new Map<
    string,
    Promise<SnapConnection>
  >()
  private readonly container: HTMLElement
  private nextCommandId = 0

  constructor(container?: HTMLElement) {
    this.container = container ?? document.body
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
      // Cross-scheme chrome:// -> chrome-untrusted:// keeps isolation; without
      // allow-same-origin the frame gets an opaque origin and its postMessage
      // arrives with origin "null", failing every origin check below.
      iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
      iframe.style.display = 'none'
      iframe.src = SNAP_EXECUTOR_URL

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
          event.origin !== SNAP_EXECUTOR_ORIGIN
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
        fail(new Error('Snap executor handshake timed out'))
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
          event.origin !== SNAP_EXECUTOR_ORIGIN
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

      window.addEventListener('message', handler)
      conn.iframe.contentWindow.postMessage(
        {
          type: command.type,
          requestId,
          payload: command.payload,
        },
        SNAP_EXECUTOR_ORIGIN,
      )

      const timer = window.setTimeout(() => {
        settle(() =>
          reject(new Error(`Snap command '${command.type}' timed out`)),
        )
      }, COMMAND_TIMEOUT_MS)
    })
  }
}
