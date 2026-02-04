// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Service Worker for running Embedding Gemma WASM model.
// Uses static imports (dynamic import() is disallowed in Service Workers).
// The "web" target from wasm-pack generates an init() function instead of
// using top-level await, making it compatible with Service Workers.

// Static import of the web-target WASM module.
// This is fetched and cached during SW installation.
import init, {
  Gemma3Embedder,
} from 'chrome-untrusted://candle-embedding-gemma-wasm/wasm/index.js'

console.log('[Candle SW] Service Worker script loaded')

// State
let embedder = null
let isInitialized = false
let isInitializing = false
let messagePort = null

// Decode base64 string to Uint8Array
function base64ToUint8Array(base64) {
  const binaryString = atob(base64)
  const bytes = new Uint8Array(binaryString.length)
  for (let i = 0; i < binaryString.length; i++) {
    bytes[i] = binaryString.charCodeAt(i)
  }
  return bytes
}

// Handle init-with-data message - receive files via MessagePort and initialize
// Files are base64-encoded to work with MessagePort string transport.
async function handleInitWithData(data) {
  if (isInitialized) {
    console.log('[Candle SW] Model already initialized')
    return true
  }

  if (isInitializing) {
    console.log('[Candle SW] Initialization already in progress')
    return false
  }

  isInitializing = true
  const startTime = performance.now()
  console.log('[Candle SW] Starting initialization with transferred data...')

  try {
    // Check if data contains base64-encoded files
    if (!data.weights || !data.dense1 || !data.dense2 ||
        !data.tokenizer || !data.config || !data.wasm) {
      console.error('[Candle SW] Missing required file data')
      isInitializing = false
      return false
    }

    console.log('[Candle SW] Decoding base64 data...')
    const decodeStart = performance.now()

    const weightsBytes = base64ToUint8Array(data.weights)
    const dense1Bytes = base64ToUint8Array(data.dense1)
    const dense2Bytes = base64ToUint8Array(data.dense2)
    const tokenizerBytes = base64ToUint8Array(data.tokenizer)
    const configBytes = base64ToUint8Array(data.config)
    const wasmBytes = base64ToUint8Array(data.wasm)

    const decodeTime = performance.now() - decodeStart
    console.log(`[Candle SW] Base64 decode time: ${decodeTime.toFixed(2)}ms`)

    console.log('[Candle SW] Received data:')
    console.log('  weights:', weightsBytes.length, 'bytes')
    console.log('  dense1:', dense1Bytes.length, 'bytes')
    console.log('  dense2:', dense2Bytes.length, 'bytes')
    console.log('  tokenizer:', tokenizerBytes.length, 'bytes')
    console.log('  config:', configBytes.length, 'bytes')
    console.log('  wasm:', wasmBytes.length, 'bytes')

    // Initialize WASM with the decoded binary
    console.log('[Candle SW] Initializing WASM from decoded buffer...')
    await init(wasmBytes.buffer)
    console.log(
      '[Candle SW] WASM initialized, Gemma3Embedder:',
      !!Gemma3Embedder,
    )

    // Create embedder with the decoded model files
    console.log('[Candle SW] Creating Gemma3Embedder instance...')
    embedder = new Gemma3Embedder(
      weightsBytes,
      dense1Bytes,
      dense2Bytes,
      tokenizerBytes,
      configBytes,
    )

    isInitialized = true
    isInitializing = false
    const initTime = performance.now() - startTime
    console.log(
      `[Candle SW] Model initialized successfully in ${initTime.toFixed(2)}ms`,
    )
    return true
  } catch (error) {
    console.error('[Candle SW] Failed to initialize:', error)
    isInitializing = false
    return false
  }
}

// Handle embed message - generate embeddings for input text
function handleEmbed(input) {
  if (!isInitialized || !embedder) {
    console.error('[Candle SW] Model not initialized')
    return []
  }

  try {
    console.log(
      '[Candle SW] Running embedding on:',
      input.substring(0, 50) + '...',
    )
    const embedding = embedder.embed(input)
    console.log('[Candle SW] Embedding generated, length:', embedding.length)
    return Array.from(embedding)
  } catch (error) {
    console.error('[Candle SW] Error running embedding:', error)
    return []
  }
}

// Handle port messages
function setupPortMessageHandler(port) {
  port.onmessage = async (event) => {
    console.log(
      '[Candle SW] Received port message, data type:',
      typeof event.data,
    )

    let data

    // Handle different message formats
    if (typeof event.data === 'string') {
      try {
        data = JSON.parse(event.data)
      } catch {
        data = { raw: event.data }
      }
    } else if (typeof event.data === 'object' && event.data !== null) {
      data = event.data
    } else {
      data = { raw: event.data }
    }

    const { type, id } = data
    console.log('[Candle SW] Port message type:', type, 'id:', id)

    let response

    switch (type) {
      case 'init-with-data': {
        // Data contains base64-encoded files
        const success = await handleInitWithData(data)
        response = { type: 'init-response', id: id || 0, success }
        break
      }
      case 'embed': {
        const output = handleEmbed(data.text)
        response = { type: 'embed-response', id, output }
        break
      }
      default:
        console.warn('[Candle SW] Unknown message type:', type)
        response = { type: 'error', id: id || 0, success: false }
    }

    port.postMessage(JSON.stringify(response))
  }
}

// Handle incoming messages from browser (via StartServiceWorkerAndDispatchMessage)
self.addEventListener('message', (event) => {
  console.log('[Candle SW] Received message event')
  console.log('[Candle SW] event.ports:', event.ports?.length || 0)

  // Check if a MessagePort was sent for bidirectional communication
  if (event.ports && event.ports.length > 0) {
    messagePort = event.ports[0]
    console.log('[Candle SW] MessagePort received')

    setupPortMessageHandler(messagePort)

    // Acknowledge port connection
    messagePort.postMessage(JSON.stringify({ type: 'connected' }))
    return
  }

  console.log('[Candle SW] Message without ports - ignoring')
})

// Service Worker lifecycle events
self.addEventListener('install', (event) => {
  console.log('[Candle SW] Installing...')
  event.waitUntil(self.skipWaiting())
})

self.addEventListener('activate', (event) => {
  console.log('[Candle SW] Activating...')
  event.waitUntil(self.clients.claim())
})

console.log('[Candle SW] Service Worker setup complete')
