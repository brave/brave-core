// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// SnapBridge: manages snap iframes and bridges between C++ SnapController
// and chrome-untrusted://snap-executor/ iframes via Mojo + MetaMask streams.
//
// Architecture:
//   C++ SnapController  <--Mojo-->  SnapBridge (this)  <--PostMessageStream-->  iframe(s)
//
// Communication with iframes uses MetaMask's @metamask/post-message-stream and
// @metamask/object-multiplex with JSON-RPC 2.0 protocol, matching what
// IFrameSnapExecutor expects.

import { BraveWallet } from '../../constants/types'
import { getAPIProxy } from '../async/bridge'
import type { Value as MojoValue } from 'chrome://resources/mojo/mojo/public/mojom/base/values.mojom-webui.js'
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore — types resolve after npm install
import { WindowPostMessageStream } from '@metamask/post-message-stream'
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import ObjectMultiplex from '@metamask/object-multiplex'

// Using 'any' for stream types since readable-stream lacks type declarations
// in this build environment. The runtime types are correct.
// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Duplex = any

const SNAP_EXECUTOR_ORIGIN = 'chrome-untrusted://snap-executor'

// Bundle URLs for built-in snaps — fetched by the bridge and passed as
// sourceCode to executeSnap.
const SNAP_BUNDLE_URLS: Record<string, string> = {
  'npm:@cosmsnap/snap': 'snap-bundles/cosmos.js',
  'npm:filsnap': 'snap-bundles/filecoin.js',
  'npm:@polkagate/snap': 'snap-bundles/polkadot.js',
}

// Base endowments always provided to every snap (factory-backed in the executor).
const BASE_ENDOWMENTS = [
  'console', 'crypto', 'SubtleCrypto', 'Date', 'Math',
  'setTimeout', 'clearTimeout', 'setInterval', 'clearInterval',
  'TextEncoder', 'TextDecoder',
]

// Additional endowments keyed by snap initialPermission name.
// Non-factory globals are read from rootRealmGlobal (captured before LavaMoat
// scuttles globalThis), so they are safe to request here.
const PERMISSION_ENDOWMENTS: Record<string, string[]> = {
  'endowment:network-access': [
    'fetch', 'Request', 'Headers', 'Response',
    'URL', 'URLSearchParams', 'AbortController', 'AbortSignal',
    'atob', 'btoa', 'WebSocket',
  ],
  'endowment:webassembly': ['WebAssembly'],
}

// Per-snap endowments derived from manifest initialPermissions.
// Typed arrays and ArrayBuffer are always included since many crypto/encoding
// libs (used by all three snaps) depend on them.
const COMMON_GLOBALS = [
  'Uint8Array', 'Int8Array', 'Uint16Array', 'Int16Array',
  'Uint32Array', 'Int32Array', 'Float32Array', 'Float64Array',
  'BigInt64Array', 'BigUint64Array', 'ArrayBuffer', 'DataView',
]

const SNAP_PERMISSIONS: Record<string, string[]> = {
  'npm:@cosmsnap/snap':    ['endowment:network-access'],
  'npm:filsnap':           ['endowment:network-access'],
  'npm:@polkagate/snap':   ['endowment:network-access', 'endowment:webassembly'],
}

// Endowment permissions for dynamically installed snaps, populated at load
// time from C++ via GetInstalledSnaps().
const installedSnapPermissions = new Map<string, string[]>()

function getEndowmentsForSnap(snapId: string): string[] {
  // Built-in snaps use the hardcoded map.
  let permissions = SNAP_PERMISSIONS[snapId]

  if (!permissions) {
    // Installed snap — use the cached endowment permissions.
    permissions = installedSnapPermissions.get(snapId) ?? []
  }

  const extras = permissions.flatMap((p) => PERMISSION_ENDOWMENTS[p] ?? [])
  return [...new Set([...BASE_ENDOWMENTS, ...extras, ...COMMON_GLOBALS])]
}

// Parameters passed to the dialog handler when a snap calls snap_dialog or snap_confirm.
export interface SnapDialogParams {
  dialogType: string
  content?: unknown
  prompt?: string
  description?: string
  textAreaContent?: string
}

// Per-snap connection state.
interface SnapConnection {
  iframe: HTMLIFrameElement
  commandStream: Duplex
  rpcStream: Duplex
  // In-memory interface store for snap_createInterface / snap_updateInterface.
  interfaces: Map<string, unknown>
  // In-memory form state for snap_getInterfaceState.
  interfaceStates: Map<string, Record<string, string>>
}

// Convert a mojo_base.mojom.Value tagged union to a plain JS value.
export function mojoValueToJs(v: MojoValue | null | undefined): unknown {
  if (v === null || v === undefined) { return null }
  if (v.nullValue !== undefined) { return null }
  if (v.boolValue !== undefined) { return v.boolValue }
  if (v.intValue !== undefined) { return v.intValue }
  if (v.doubleValue !== undefined) { return v.doubleValue }
  if (v.stringValue !== undefined) { return v.stringValue }
  if (v.listValue !== undefined) {
    return v.listValue.storage.map((item: MojoValue) => mojoValueToJs(item))
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
export function jsToMojoValue(v: unknown): MojoValue {
  if (v === null || v === undefined) { return { nullValue: 0 } }
  if (typeof v === 'boolean') { return { boolValue: v } }
  if (typeof v === 'number') {
    return Number.isInteger(v) ? { intValue: v } : { doubleValue: v }
  }
  if (typeof v === 'string') { return { stringValue: v } }
  if (Array.isArray(v)) {
    return { listValue: { storage: v.map((item) => jsToMojoValue(item)) } }
  }
  if (typeof v === 'object') {
    const storage: Record<string, MojoValue> = {}
    for (const [k, val] of Object.entries(v as Record<string, unknown>)) {
      storage[k] = jsToMojoValue(val)
    }
    return { dictionaryValue: { storage } }
  }
  return { nullValue: 0 }
}

let rpcIdCounter = 1

export class SnapBridge {
  // One connection per snap, keyed by snapId.
  private readonly connections = new Map<string, SnapConnection>()

  // Remote to call C++ SnapRequestHandler when snap code calls snap.request().
  private snapRequestHandler: BraveWallet.SnapRequestHandlerRemote | null = null

  // The Mojo receiver wrapping this class.
  private receiver: BraveWallet.SnapBridgeReceiver | null = null

  // Handler for snap_dialog / snap_confirm.
  private dialogHandler:
    | ((params: SnapDialogParams) => Promise<boolean>)
    | null = null

  // Element to which snap iframes are appended.
  private readonly container: HTMLElement

  constructor(container?: HTMLElement) {
    this.container = container ?? document.body
  }

  setSnapRequestHandler(
    handler: BraveWallet.SnapRequestHandlerRemote,
  ): void {
    this.snapRequestHandler = handler
  }

  setDialogHandler(
    handler: (params: SnapDialogParams) => Promise<boolean>,
  ): void {
    this.dialogHandler = handler
  }

  bindNewPipeAndPassRemote() {
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    this.receiver = new BraveWallet.SnapBridgeReceiver(this as any)
    return this.receiver.$.bindNewPipeAndPassRemote()
  }

  // -------------------------------------------------------------------------
  // SnapBridge Mojo interface — called by C++ SnapController
  // -------------------------------------------------------------------------

  async loadSnap(
    snapId: string,
  ): Promise<{ success: boolean; error: string | null }> {
    try {
      console.error('XXXZZZ SnapBridge.loadSnap', snapId)
      let conn = this.connections.get(snapId)
      if (!conn) {
        console.error('XXXZZZ SnapBridge.loadSnap: creating connection')
        conn = await this.createConnection(snapId)
        console.error('XXXZZZ SnapBridge.loadSnap: connection created')
      }

      const bundleUrl = SNAP_BUNDLE_URLS[snapId]
      let sourceCode: string
      if (bundleUrl) {
        // Built-in snap: bundle is served from chrome-untrusted://snap-executor/
        console.error('XXXZZZ SnapBridge.loadSnap: fetching built-in bundle', bundleUrl)
        sourceCode = await this.fetchBundleFromIframe(conn, bundleUrl)
        console.error('XXXZZZ SnapBridge.loadSnap: built-in bundle fetched, length=', sourceCode.length)
      } else {
        // Installed snap: fetch bundle from C++ file storage via Mojo.
        console.error('XXXZZZ SnapBridge.loadSnap: fetching installed snap bundle', snapId)
        const { braveWalletService } = getAPIProxy()
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        const svc = braveWalletService as any
        const { sourceCode: code, error } = await svc.getSnapBundle(snapId)
        if (error || !code) {
          return { success: false, error: error ?? 'Bundle not found' }
        }
        sourceCode = code as string

        // Cache the snap's endowment permissions for getEndowmentsForSnap().
        const { snaps } = await svc.getInstalledSnaps()
        for (const snap of snaps) {
          installedSnapPermissions.set(
            snap.snapId,
            (snap.permissions as string[]).filter((p: string) => p.startsWith('endowment:')),
          )
        }
        console.error('XXXZZZ SnapBridge.loadSnap: installed bundle fetched, length=', sourceCode.length)
      }

      console.error('XXXZZZ SnapBridge.loadSnap: sending executeSnap command')
      await this.sendCommand(conn.commandStream, 'executeSnap', {
        snapId,
        sourceCode,
        endowments: getEndowmentsForSnap(snapId),
      })
      console.error('XXXZZZ SnapBridge.loadSnap: executeSnap done')

      return { success: true, error: null }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      console.error('XXXZZZ SnapBridge.loadSnap ERROR:', msg)
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

      const plainParams = mojoValueToJs(params as MojoValue)

      const result = await this.sendCommand(conn.commandStream, 'snapRpc', {
        snapId,
        handler: 'onRpcRequest',
        origin: callerOrigin,
        request: {
          jsonrpc: '2.0',
          method,
          params: plainParams,
        },
      })

      const mojoResult = result !== undefined && result !== null
        ? jsToMojoValue(result)
        : null
      return { result: mojoResult, error: null }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      return { result: null, error: msg }
    }
  }

  // Proxy an HTTP GET through the snap executor iframe (which has connect-src *).
  async proxyFetch(_snapId: string, url: string): Promise<string> {
    // With the MetaMask executor, the iframe doesn't handle proxy_fetch directly.
    // Instead, we use a temporary iframe approach or rely on the snap's own fetch.
    // For now, use a simple XMLHttpRequest from the wallet page if possible,
    // or fall back to creating a hidden iframe.
    const resp = await fetch(url)
    return resp.text()
  }

  async getHomePage(snapId: string): Promise<{ content: unknown; interfaceId?: string; error?: string }> {
    try {
      console.error('XXXZZZ SnapBridge.getHomePage', snapId)
      let conn = this.connections.get(snapId)
      if (!conn) {
        conn = await this.createConnection(snapId)
        const loadResult = await this.loadSnap(snapId)
        if (!loadResult.success) {
          return { content: null, error: loadResult.error ?? 'Failed to load snap' }
        }
      }

      console.error('XXXZZZ SnapBridge.getHomePage: sending snapRpc onHomePage')
      const result = await this.sendCommand(conn.commandStream, 'snapRpc', {
        snapId,
        handler: 'onHomePage',
        origin: this.getCallerOrigin(),
        request: {
          jsonrpc: '2.0',
          method: '',
          params: [],
        },
      }) as Record<string, unknown> | null
      console.error('XXXZZZ SnapBridge.getHomePage: result=', JSON.stringify(result)?.slice(0, 300))

      if (!result) {
        return { content: null }
      }

      if (typeof result.id === 'string' && !result.content) {
        const content = conn.interfaces.get(result.id)
        console.error('XXXZZZ SnapBridge.getHomePage: resolved interface id=', result.id, 'content=', !!content)
        return { content, interfaceId: result.id as string }
      }

      return { content: result.content ?? result }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      console.error('XXXZZZ SnapBridge.getHomePage ERROR:', msg)
      return { content: null, error: msg }
    }
  }

  async sendUserInput(
    snapId: string,
    interfaceId: string,
    event: object,
  ): Promise<{ content: unknown; error?: string }> {
    try {
      const conn = this.connections.get(snapId)
      if (!conn) {
        return { content: null, error: `Snap '${snapId}' is not loaded` }
      }

      await this.sendCommand(conn.commandStream, 'snapRpc', {
        snapId,
        handler: 'onUserInput',
        origin: this.getCallerOrigin(),
        request: {
          jsonrpc: '2.0',
          method: '',
          params: { id: interfaceId, event },
        },
      })

      // After onUserInput, the snap may have called snap_updateInterface.
      // Return the current content from our interface store.
      const content = conn.interfaces.get(interfaceId)
      return { content }
    } catch (err) {
      const msg = err instanceof Error ? err.message : String(err)
      return { content: null, error: msg }
    }
  }

  unloadSnap(snapId: string): void {
    const conn = this.connections.get(snapId)
    if (conn) {
      conn.iframe.remove()
      this.connections.delete(snapId)
    }
  }

  // Called by C++ on behalf of the panel — loads the snap and returns the
  // homepage component tree as a JSON string.
  async fetchSnapHomePage(snapId: string): Promise<{
    contentJson: string | null
    interfaceId: string | null
    error: string | null
  }> {
    const result = await this.getHomePage(snapId)
    if (result.error) {
      return { contentJson: null, interfaceId: null, error: result.error }
    }
    return {
      contentJson: result.content ? JSON.stringify(result.content) : null,
      interfaceId: result.interfaceId ?? null,
      error: null,
    }
  }

  // Called by C++ on behalf of the panel — forwards a user input event and
  // returns the updated component tree as a JSON string.
  async sendSnapUserInputEvent(
    snapId: string,
    interfaceId: string,
    eventJson: string,
  ): Promise<{ contentJson: string | null; error: string | null }> {
    try {
      const event = JSON.parse(eventJson) as Parameters<typeof this.sendUserInput>[2]
      const result = await this.sendUserInput(snapId, interfaceId, event)
      if (result.error) {
        return { contentJson: null, error: result.error }
      }
      return {
        contentJson: result.content ? JSON.stringify(result.content) : null,
        error: null,
      }
    } catch (err) {
      return { contentJson: null, error: String(err) }
    }
  }

  // -------------------------------------------------------------------------
  // Private helpers
  // -------------------------------------------------------------------------

  private async createConnection(snapId: string): Promise<SnapConnection> {
    console.error('XXXZZZ SnapBridge.createConnection', snapId)
    const iframe = await this.createAndWaitForIframe(snapId)
    console.error('XXXZZZ SnapBridge.createConnection: iframe ready')

    // Create a PostMessageStream to the iframe (parent side of the pair).
    // The child side is created by IFrameSnapExecutor.initialize() with
    // name='child', target='parent'.
    const stream = new WindowPostMessageStream({
      name: 'parent',
      target: 'child',
      targetOrigin: SNAP_EXECUTOR_ORIGIN,
      targetWindow: iframe.contentWindow!,
    })

    // Create ObjectMultiplex and pipe through the stream.
    const mux = new ObjectMultiplex()
    // pipe() instead of pipeline() — pipeline is not available in the browser
    // webpack build. Bidirectional piping: stream → mux → stream.
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    ;(stream as any).pipe(mux)
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    ;(mux as any).pipe(stream)

    // Create the two top-level sub-streams that BaseSnapExecutor expects.
    const commandStream = mux.createStream('command') as Duplex

    // The jsonRpc sub-stream is itself another ObjectMultiplex.
    // snap.request() calls arrive on the 'metamask-provider' sub-sub-stream.
    const jsonRpcRaw = mux.createStream('jsonRpc') as Duplex
    const rpcMux = new ObjectMultiplex()
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    ;(jsonRpcRaw as any).pipe(rpcMux)
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    ;(rpcMux as any).pipe(jsonRpcRaw)
    // The snap provider sub-stream — JSON-RPC messages from snap.request().
    const rpcStream = rpcMux.createStream('metamask-provider') as Duplex

    const conn: SnapConnection = {
      iframe,
      commandStream,
      rpcStream,
      interfaces: new Map(),
      interfaceStates: new Map(),
    }

    // Listen for snap.request() calls on the rpcStream.
    rpcStream.on('data', (data: unknown) => {
      const msg = data as Record<string, unknown>
      console.error('XXXZZZ rpcStream data:', msg?.method, 'id=', msg?.id)
      void this.handleRpcRequest(snapId, conn, data)
    })
    rpcStream.on('error', (err: unknown) => {
      console.error('XXXZZZ rpcStream error:', err)
    })
    commandStream.on('error', (err: unknown) => {
      console.error('XXXZZZ commandStream error:', err)
    })

    // Debug: log all raw messages from the iframe to confirm postMessage flow
    window.addEventListener('message', (ev: MessageEvent) => {
      if (ev.source === iframe.contentWindow) {
        console.error('XXXZZZ raw iframe msg:', JSON.stringify(ev.data)?.slice(0, 800))
      }
    })

    this.connections.set(snapId, conn)
    console.error('XXXZZZ SnapBridge.createConnection: streams ready, waiting for handshake...')
    return conn
  }

  private createAndWaitForIframe(snapId: string): Promise<HTMLIFrameElement> {
    return new Promise((resolve) => {
      const iframe = document.createElement('iframe')
      iframe.src = `chrome-untrusted://snap-executor/?snap_id=${encodeURIComponent(snapId)}`
      iframe.setAttribute('sandbox', 'allow-scripts allow-same-origin')
      iframe.style.display = 'none'
      iframe.addEventListener('load', () => resolve(iframe), { once: true })
      this.container.appendChild(iframe)
      console.error('XXXZZZ SnapBridge: iframe attached to DOM for', snapId)
    })
  }

  // Fetches a bundle from the snap executor origin via a hidden fetch through
  // the iframe's connect-src * CSP. We POST a fetch request and wait for response.
  // Ask the snap executor iframe to fetch the bundle from its own
  // chrome-untrusted://snap-executor/ origin (which has connect-src *).
  // The wallet page cannot fetch chrome-untrusted:// URLs directly.
  // The snap_prefetch_bridge.bundle.js shim handles the 'brave_prefetch_bundle' message.
  private fetchBundleFromIframe(
    conn: SnapConnection,
    bundleUrl: string,
  ): Promise<string> {
    return new Promise((resolve, reject) => {
      const requestId = `prefetch-${Date.now()}-${Math.random().toString(36).slice(2)}`
      const fullUrl = `${SNAP_EXECUTOR_ORIGIN}/${bundleUrl}`

      const onMessage = (event: MessageEvent) => {
        if (event.origin !== SNAP_EXECUTOR_ORIGIN) {
          return
        }
        const data = event.data as Record<string, unknown>
        if (data?.type !== 'brave_prefetch_bundle_response' || data.requestId !== requestId) {
          return
        }
        window.removeEventListener('message', onMessage)
        if (data.error) {
          reject(new Error(data.error as string))
        } else {
          resolve(data.source as string)
        }
      }

      window.addEventListener('message', onMessage)
      conn.iframe.contentWindow?.postMessage(
        { type: 'brave_prefetch_bundle', url: fullUrl, requestId },
        SNAP_EXECUTOR_ORIGIN,
      )
    })
  }

  // Sends a JSON-RPC 2.0 command on the command stream and waits for a response.
  private sendCommand(
    commandStream: Duplex,
    method: string,
    params: unknown,
  ): Promise<unknown> {
    return new Promise((resolve, reject) => {
      const id = rpcIdCounter++

      const onData = (data: unknown) => {
        const msg = data as { id?: number; result?: unknown; error?: { message?: string; code?: number } }
        if (msg.id !== id) {
          return
        }
        commandStream.removeListener('data', onData)
        if (msg.error) {
          reject(new Error(msg.error.message ?? 'Unknown executor error'))
        } else {
          resolve(msg.result)
        }
      }

      console.error('XXXZZZ SnapBridge.sendCommand:', method, 'id=', id)
      commandStream.on('data', onData)
      commandStream.write({
        jsonrpc: '2.0',
        id,
        method,
        params,
      })
      console.error('XXXZZZ SnapBridge.sendCommand: message written, waiting for response...')
    })
  }

  private getCallerOrigin(): string {
    return window.location.origin
  }

  // Handles JSON-RPC requests arriving on the rpcStream — these are
  // snap.request() calls from the snap code inside the Compartment.
  private async handleRpcRequest(
    snapId: string,
    conn: SnapConnection,
    data: unknown,
  ): Promise<void> {
    const msg = data as {
      id?: number | string
      method?: string
      params?: unknown
    }
    if (!msg.id || !msg.method) {
      console.error('XXXZZZ handleRpcRequest: skipping (no id or method)', msg)
      return
    }

    console.error('XXXZZZ handleRpcRequest:', msg.method, 'id=', msg.id)

    const respond = (result?: unknown, error?: { code: number; message: string }) => {
      console.error('XXXZZZ handleRpcRequest respond:', msg.method, error ? 'ERROR:'+error.message : 'OK')
      const response: Record<string, unknown> = { jsonrpc: '2.0', id: msg.id }
      if (error) {
        response.error = error
      } else {
        response.result = result ?? null
      }
      conn.rpcStream.write(response)
    }

    try {
      const result = await this.dispatchSnapRequest(snapId, conn, msg.method, msg.params)
      respond(result)
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err)
      respond(undefined, { code: -32603, message: errMsg })
    }
  }

  // Routes snap.request() calls — some handled locally, others forwarded to C++.
  private async dispatchSnapRequest(
    snapId: string,
    conn: SnapConnection,
    method: string,
    params: unknown,
  ): Promise<unknown> {
    if (method === 'snap_createInterface') {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown> | undefined
      const ui = p?.ui ?? p
      const interfaceId = `${Date.now()}-${Math.random().toString(36).slice(2)}`
      conn.interfaces.set(interfaceId, ui)
      return interfaceId
    }

    if (method === 'snap_updateInterface') {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown> | undefined
      const id = p?.id as string | undefined
      const ui = p?.ui
      if (id) {
        conn.interfaces.set(id, ui)
      }
      return null
    }

    if (method === 'snap_getInterfaceState') {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown> | undefined
      const id = p?.id as string | undefined
      return id ? (conn.interfaceStates.get(id) ?? {}) : {}
    }

    // --- snap_dialog / snap_confirm — render UI ---

    if (method === 'snap_dialog' || method === 'snap_confirm') {
      return this.handleSnapDialog(method, params)
    }

    // --- Forward to C++ ---

    if (!this.snapRequestHandler) {
      throw new Error('SnapRequestHandler not connected')
    }

    // snap_manageState: serialize newState to JSON string before sending to
    // C++ (which stores raw bytes), and parse the returned JSON string back.
    if (method === 'snap_manageState') {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown> | undefined
      const operation = p?.operation as string | undefined
      const mojoParams: Record<string, unknown> = { operation }
      if (operation === 'update') {
        mojoParams.newStateJson = JSON.stringify(p?.newState ?? null)
      }
      const { result, error } = await this.snapRequestHandler.handleSnapRequest(
        snapId, method, jsToMojoValue(mojoParams) as any)
      if (error) throw new Error(error)
      const raw = mojoValueToJs(result ?? null) as string | null
      if (!raw || raw === 'null') return null
      return JSON.parse(raw)
    }

    let plainParams: unknown = {}
    try {
      plainParams = JSON.parse(JSON.stringify(params ?? {}))
    } catch {
      plainParams = {}
    }

    const { result, error } = await this.snapRequestHandler.handleSnapRequest(
      snapId,
      method,
      jsToMojoValue(plainParams) as any,
    )
    if (error) {
      throw new Error(error)
    }
    const jsResult = mojoValueToJs(result ?? null)
    if (method === 'snap_getBip44Entropy') {
      console.error('XXXZZZ snap_getBip44Entropy result:', JSON.stringify(jsResult))
    }
    return jsResult
  }

  private async handleSnapDialog(
    method: string,
    params: unknown,
  ): Promise<boolean> {
    if (!this.dialogHandler) {
      console.warn('SnapBridge: no dialogHandler registered, auto-confirming')
      return true
    }

    let dialogParams: SnapDialogParams

    if (method === 'snap_confirm') {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown>
      dialogParams = {
        dialogType: 'confirmation',
        prompt: p?.prompt as string | undefined,
        description: p?.description as string | undefined,
        textAreaContent: p?.textAreaContent as string | undefined,
      }
    } else {
      const p = (Array.isArray(params) ? params[0] : params) as Record<string, unknown>
      dialogParams = {
        dialogType: (p?.type as string) ?? 'confirmation',
        content: p?.content,
      }
    }

    return this.dialogHandler(dialogParams)
  }
}
