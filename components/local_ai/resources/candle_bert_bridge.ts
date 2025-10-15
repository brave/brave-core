// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BertInterfaceReceiver,
  CandleService,
} from 'gen/brave/components/local_ai/common/candle.mojom.m.js'

console.log('[Candle WASM] Script loaded')

// Import the WASM Model class from candle_bert
// The candle_bert module is built separately and made available via resources
// @ts-expect-error - Module path will be resolved at runtime via resources
import { Model } from 'chrome-untrusted://resources/brave/candle_bert.bundle.js'

console.log('[Candle WASM] Initializing Mojo connection...')

// Initialize connection to the browser-side CandleService
const candleService = CandleService.getRemote()

console.log('[Candle WASM] CandleService remote obtained:', candleService)

// Cache for fetching model files
async function fetchArrayBuffer(url: string): Promise<Uint8Array> {
  const cacheName = 'bert-candle-cache'
  const cache = await caches.open(cacheName)
  const cachedResponse = await cache.match(url)
  if (cachedResponse) {
    const data = await cachedResponse.arrayBuffer()
    return new Uint8Array(data)
  }
  const res = await fetch(url, { cache: 'force-cache' })
  cache.put(url, res.clone())
  return new Uint8Array(await res.arrayBuffer())
}

// Implement the BertInterface Mojo observer
class BertInterfaceImpl {
  receiver: BertInterfaceReceiver
  model: typeof Model | null
  isInitialized: boolean

  constructor() {
    this.receiver = new BertInterfaceReceiver(this)
    this.model = null
    this.isInitialized = false
  }

  // Initialize WASM module
  async initialize(): Promise<void> {
    if (this.isInitialized) {
      return
    }

    console.log('Initializing candle_bert WASM module...')
    // WASM module is auto-initialized on import
    this.isInitialized = true
    console.log('WASM module initialized successfully!')
  }

  // Load the BERT model
  async loadModel(
    weightsURL: string,
    tokenizerURL: string,
    configURL: string,
  ): Promise<void> {
    await this.initialize()

    console.log('Loading BERT model...')
    const [weightsArrayU8, tokenizerArrayU8, configArrayU8] = await Promise.all(
      [
        fetchArrayBuffer(weightsURL),
        fetchArrayBuffer(tokenizerURL),
        fetchArrayBuffer(configURL),
      ],
    )

    this.model = new Model(weightsArrayU8, tokenizerArrayU8, configArrayU8)
    console.log('BERT model loaded successfully!')
  }

  // Implementation of BertInterface::RunExample
  async runExample(): Promise<{ result: string }> {
    await this.initialize()

    if (!this.model) {
      console.log('Model not loaded, loading default model...')
      // Model files are served from chrome-untrusted://candle-bert-wasm
      const weightsURL =
        'chrome-untrusted://candle-bert-wasm/bert/model.safetensors'
      const tokenizerURL =
        'chrome-untrusted://candle-bert-wasm/bert/tokenizer.json'
      const configURL = 'chrome-untrusted://candle-bert-wasm/bert/config.json'

      try {
        await this.loadModel(weightsURL, tokenizerURL, configURL)
      } catch (error) {
        console.error('Failed to load model:', error)
        return { result: `Error loading model: ${error}` }
      }
    }

    try {
      // Run a simple embedding example
      const sentences = [
        'The cat sits outside',
        'A man is playing guitar',
        'I love pasta',
        'The new movie is awesome',
      ]

      console.log('Running embedding inference on sample sentences...')
      const output = this.model!.get_embeddings({
        sentences: sentences,
        normalize_embeddings: true,
      })

      console.log('Embeddings generated!', output)

      // Calculate similarity between first and other sentences
      const embeddings = output.data
      const similarities = []
      for (let i = 1; i < embeddings.length; i++) {
        const similarity = cosineSimilarity(embeddings[0], embeddings[i])
        similarities.push(`"${sentences[i]}": ${similarity.toFixed(4)}`)
      }

      const result = `BERT Embeddings Test:\n\nReference: "${sentences[0]}"\n\nSimilarities:\n${similarities.join('\n')}`
      return { result }
    } catch (error) {
      console.error('Error running example:', error)
      return { result: `Error: ${error}` }
    }
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Cosine similarity helper function
function cosineSimilarity(a: number[], b: number[]): number {
  let dotProduct = 0
  let normA = 0
  let normB = 0
  for (let i = 0; i < a.length; i++) {
    dotProduct += a[i] * b[i]
    normA += a[i] * a[i]
    normB += b[i] * b[i]
  }
  return dotProduct / (Math.sqrt(normA) * Math.sqrt(normB))
}

// Create and register the BertInterface implementation
console.log('[Candle WASM] Creating BertInterface implementation...')
const bertImpl = new BertInterfaceImpl()
console.log('[Candle WASM] Binding BertInterface to CandleService...')
candleService.bindBert(bertImpl.getPendingRemote())

console.log('[Candle WASM] WASM bridge initialized successfully!')

// Optionally expose to window for debugging
;(window as any).bertImpl = bertImpl
console.log('[Candle WASM] Exposed bertImpl to window for debugging')
