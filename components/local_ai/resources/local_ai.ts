// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  LocalAIService,
  PassageEmbedderFactoryReceiver,
  PassageEmbedderInterface,
  PassageEmbedderFactoryInterface,
  PassageEmbedderPendingReceiver,
  PassageEmbedderReceiver,
  ModelFiles,
} from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'

// Minimal type for the WASM Gemma3Embedder class
interface Gemma3EmbedderInstance {
  embed(input: string): Float32Array
}

// Initialize connection to the browser-side LocalAIService
const localAIService = LocalAIService.getRemote()

class PassageEmbedderImpl implements PassageEmbedderInterface {
  embedder: Gemma3EmbedderInstance

  constructor(embedder: Gemma3EmbedderInstance) {
    this.embedder = embedder
  }

  async generateEmbeddings(text: string) {
    try {
      const embedding = this.embedder.embed(text)
      // Convert Float32Array to number[] (array<double> in mojo)
      return { embedding: Array.from(embedding) }
    } catch (error) {
      console.error('Error running embedding:', error)
      return { embedding: [] }
    }
  }
}

class PassageEmbedderFactoryImpl implements PassageEmbedderFactoryInterface {
  receiver: PassageEmbedderFactoryReceiver
  embedder: Gemma3EmbedderInstance | null
  isInitialized: boolean
  initTime: number

  constructor() {
    this.receiver = new PassageEmbedderFactoryReceiver(this)
    this.embedder = null
    this.isInitialized = false
    this.initTime = 0
  }

  async init(modelFiles: ModelFiles) {
    if (this.isInitialized) {
      return { success: true }
    }

    const startTime = performance.now()

    try {
      // Helper function to extract data from BigBuffer.
      // BigBuffer is a union: { bytes?: number[] } |
      // { sharedMemory?: { bufferHandle, size } }.
      const extractBigBuffer = (
        buffer: {
          bytes?: number[]
          sharedMemory?: {
            bufferHandle: {
              mapBuffer(o: number, s: number): { buffer: ArrayBuffer | null }
            }
            size: number
          }
        },
        name: string,
      ): Uint8Array | null => {
        if (buffer.bytes) {
          return new Uint8Array(buffer.bytes)
        } else if (buffer.sharedMemory) {
          const sharedMem = buffer.sharedMemory
          const mapResult = sharedMem.bufferHandle.mapBuffer(0, sharedMem.size)
          if (!mapResult.buffer) {
            console.error(`Failed to map ${name} shared buffer`)
            return null
          }
          return new Uint8Array(mapResult.buffer)
        } else {
          console.error(`Invalid ${name} BigBuffer`)
          return null
        }
      }

      const weightsData = extractBigBuffer(modelFiles.weights, 'Weights')
      const weightsDense1Data = extractBigBuffer(
        modelFiles.weightsDense1,
        'WeightsDense1',
      )
      const weightsDense2Data = extractBigBuffer(
        modelFiles.weightsDense2,
        'WeightsDense2',
      )
      const tokenizerData = extractBigBuffer(modelFiles.tokenizer, 'Tokenizer')
      const configData = extractBigBuffer(modelFiles.config, 'Config')

      if (
        !weightsData
        || !weightsDense1Data
        || !weightsDense2Data
        || !tokenizerData
        || !configData
      ) {
        console.error('Failed to extract model files')
        return { success: false }
      }

      // Dynamically import the WASM Gemma3Embedder class.
      // Full URL required — webpack can't resolve relative dynamic
      // imports at runtime. Served by the same WebUI data source.
      const module = await import(
        // @ts-ignore
        'chrome-untrusted://local-ai/candle_embedding_gemma.bundle.js'
      )
      const { Gemma3Embedder } = module

      this.embedder = new Gemma3Embedder(
        weightsData,
        weightsDense1Data,
        weightsDense2Data,
        tokenizerData,
        configData,
      )
      this.isInitialized = true
      this.initTime = performance.now() - startTime
      console.log(`Model loaded successfully in ${this.initTime.toFixed(2)}ms`)
      return { success: true }
    } catch (error) {
      console.error('Failed to load model:', error)
      return { success: false }
    }
  }

  bind(receiver: PassageEmbedderPendingReceiver) {
    if (!this.embedder) {
      console.error('Cannot create embedder: model not initialized')
      return
    }
    const impl = new PassageEmbedderImpl(this.embedder)
    const bound = new PassageEmbedderReceiver(impl)
    bound.$.bindHandle(receiver.handle)
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Create and register the PassageEmbedderFactory implementation
const factory = new PassageEmbedderFactoryImpl()
localAIService.registerPassageEmbedderFactory(factory.getPendingRemote())

console.debug('[local-ai] Passage embedder factory initialized')
