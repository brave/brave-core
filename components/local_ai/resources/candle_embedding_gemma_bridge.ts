// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CandleService,
  EmbeddingGemmaInterfaceReceiver,
  LargeModelFiles,
} from 'gen/brave/components/local_ai/common/candle.mojom.m.js'

console.log('[Candle WASM] Embedding Gemma Script loaded')

// Import the WASM Gemma3Embedder class from candle_embedding_gemma
// @ts-expect-error - Module path will be resolved at runtime via resources
import { Gemma3Embedder } from 'chrome-untrusted://resources/brave/candle_embedding_gemma.bundle.js'

console.log('[Candle WASM] Initializing Mojo connection...')

// Initialize connection to the browser-side CandleService
const candleService = CandleService.getRemote()

console.log('[Candle WASM] CandleService remote obtained:', candleService)

// Implement the EmbeddingGemmaInterface Mojo observer
class EmbeddingGemmaInterfaceImpl {
  receiver: EmbeddingGemmaInterfaceReceiver
  embedder: typeof Gemma3Embedder | null
  isInitialized: boolean
  initTime: number

  constructor() {
    this.receiver = new EmbeddingGemmaInterfaceReceiver(this)
    this.embedder = null
    this.isInitialized = false
    this.initTime = 0
  }

  // Implementation of EmbeddingGemmaInterface::Init
  async init(modelFiles: LargeModelFiles): Promise<{ success: boolean }> {
    if (this.isInitialized) {
      console.log('Model already initialized')
      return { success: true }
    }

    const startTime = performance.now()
    console.log('Loading Embedding Gemma model from provided files...')

    try {
      // Helper function to extract data from BigBuffer
      const extractBigBuffer = (
        buffer: any,
        name: string,
      ): Uint8Array | null => {
        if (buffer.bytes) {
          const data = new Uint8Array(buffer.bytes)
          console.log(`${name} using inline bytes, size:`, data.byteLength)
          return data
        } else if (buffer.sharedMemory) {
          const sharedMem = buffer.sharedMemory
          console.log(`${name} shared memory size:`, sharedMem.size)
          const mapResult = sharedMem.bufferHandle.mapBuffer(0, sharedMem.size)
          if (!mapResult.buffer) {
            console.error(
              `Failed to map ${name} shared buffer, result:`,
              mapResult.result,
            )
            return null
          }
          const data = new Uint8Array(mapResult.buffer)
          console.log(`${name} using shared memory, size:`, data.byteLength)
          return data
        } else {
          console.error(`Invalid ${name} BigBuffer: no bytes or shared memory`)
          return null
        }
      }

      // Extract all model files from BigBuffer
      console.log('Extracting model files from BigBuffer...')
      const weightsData = extractBigBuffer(modelFiles.weights, 'Weights')
      const tokenizerData = extractBigBuffer(modelFiles.tokenizer, 'Tokenizer')
      const configData = extractBigBuffer(modelFiles.config, 'Config')

      if (!weightsData || !tokenizerData || !configData) {
        console.error('Failed to extract model files')
        return { success: false }
      }

      console.log('Creating Gemma3Embedder instance...')
      this.embedder = new Gemma3Embedder(weightsData, tokenizerData, configData)
      this.isInitialized = true
      this.initTime = performance.now() - startTime
      console.log(
        `Embedding Gemma model loaded successfully in ${this.initTime.toFixed(2)}ms`,
      )
      return { success: true }
    } catch (error) {
      console.error('Failed to load model:', error)
      return { success: false }
    }
  }

  // Implementation of EmbeddingGemmaInterface::Embed
  async embed(input: string): Promise<{ output: number[] }> {
    if (!this.isInitialized || !this.embedder) {
      console.error('Model not initialized')
      return { output: [] }
    }

    try {
      console.log('Running embedding inference on input:', input)
      const embedding = this.embedder!.embed(input)
      console.log('Embedding generated, length:', embedding.length)
      // Convert Float32Array to number[] (array<double> in mojo)
      return { output: Array.from(embedding) }
    } catch (error) {
      console.error('Error running embedding:', error)
      return { output: [] }
    }
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Create and register the EmbeddingGemmaInterface implementation
console.log('[Candle WASM] Creating EmbeddingGemmaInterface implementation...')
const embeddingGemmaImpl = new EmbeddingGemmaInterfaceImpl()
console.log('[Candle WASM] Binding EmbeddingGemmaInterface to CandleService...')
candleService.bindEmbeddingGemma(embeddingGemmaImpl.getPendingRemote())

console.log('[Candle WASM] Embedding Gemma WASM bridge initialized!')

// Optionally expose to window for debugging
;(window as any).embeddingGemmaImpl = embeddingGemmaImpl
console.log('[Candle WASM] Exposed embeddingGemmaImpl for debugging')

// UI handling
const statusEl = document.getElementById('status')!
const textInput = document.getElementById('textInput') as HTMLInputElement
const embedBtn = document.getElementById('embedBtn') as HTMLButtonElement
const resultEl = document.getElementById('result')!

// Poll for initialization
const checkInit = setInterval(() => {
  if (embeddingGemmaImpl.isInitialized) {
    statusEl.textContent = `Ready! (Initialized in ${embeddingGemmaImpl.initTime.toFixed(2)}ms)`
    statusEl.className = 'status ready'
    textInput.disabled = false
    embedBtn.disabled = false
    clearInterval(checkInit)
  }
}, 100)

// Handle embed button click
embedBtn.addEventListener('click', async () => {
  const text = textInput.value.trim()
  if (!text) return

  embedBtn.disabled = true
  const startTime = performance.now()

  try {
    const result = await candleService.embed(text)
    const embedTime = performance.now() - startTime

    if (result.embedding && result.embedding.length > 0) {
      const first5 = result.embedding
        .slice(0, 5)
        .map((v) => v.toFixed(4))
        .join(', ')
      resultEl.textContent = [
        `Text: "${text}"`,
        `Embed time: ${embedTime.toFixed(2)}ms`,
        `Dimension: ${result.embedding.length}`,
        `First 5 values: [${first5}, ...]`,
      ].join('\n')
      resultEl.style.display = 'block'
    }
  } catch (error) {
    resultEl.textContent = `Error: ${error}`
    resultEl.style.display = 'block'
  } finally {
    embedBtn.disabled = false
  }
})

// Handle example button clicks
document.querySelectorAll('.example-btn').forEach((btn) => {
  btn.addEventListener('click', () => {
    const text = btn.getAttribute('data-text')
    if (text) {
      textInput.value = text
    }
  })
})
