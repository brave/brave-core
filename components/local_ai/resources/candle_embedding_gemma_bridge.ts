// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CandleService,
  EmbeddingGemmaInterfaceReceiver,
  ModelFiles,
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

  constructor() {
    this.receiver = new EmbeddingGemmaInterfaceReceiver(this)
    this.embedder = null
    this.isInitialized = false
  }

  // Implementation of EmbeddingGemmaInterface::Init
  async init(modelFiles: ModelFiles): Promise<{ success: boolean }> {
    if (this.isInitialized) {
      console.log('Model already initialized')
      return { success: true }
    }

    console.log('Loading Embedding Gemma model from provided files...')

    try {
      // Convert mojo arrays to Vec<u8> format expected by Rust WASM
      const weights = Array.from(modelFiles.weights)
      const tokenizer = Array.from(modelFiles.tokenizer)
      const config = Array.from(modelFiles.config)

      console.log('Creating Gemma3Embedder instance...')
      this.embedder = new Gemma3Embedder(weights, tokenizer, config)
      this.isInitialized = true
      console.log('Embedding Gemma model loaded successfully!')
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
