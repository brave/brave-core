// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  LocalAIService,
  PassageEmbedderFactoryReceiver,
  PassageEmbedderReceiver,
} from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'

// Initialize connection to the browser-side LocalAIService
const localAIService = LocalAIService.getRemote()

// Implement the PassageEmbedder Mojo interface
class PassageEmbedderImpl {
  // Implementation of PassageEmbedder::GenerateEmbeddings
  async generateEmbeddings(
    _text: string,
  ): Promise<{ embedding: number[] }> {
    // TODO(https://github.com/brave/brave-browser/issues/52722):
    // stub until model loading is wired up
    return { embedding: [] }
  }
}

// Implement the PassageEmbedderFactory Mojo interface (stub).
class PassageEmbedderFactoryImpl {
  receiver: PassageEmbedderFactoryReceiver
  boundEmbedders: PassageEmbedderReceiver[]

  constructor() {
    this.receiver = new PassageEmbedderFactoryReceiver(this)
    this.boundEmbedders = []
  }

  bind(receiver: any): void {
    const impl = new PassageEmbedderImpl()
    const bound = new PassageEmbedderReceiver(impl)
    bound.$.bindHandle(receiver.handle)
    this.boundEmbedders.push(bound)
  }

  getPendingRemote() {
    return this.receiver.$.bindNewPipeAndPassRemote()
  }
}

// Create and register the PassageEmbedderFactory implementation
const factory = new PassageEmbedderFactoryImpl()
localAIService.registerPassageEmbedderFactory(
  factory.getPendingRemote(),
)

console.log('[Candle WASM] Passage embedder factory bridge initialized')
