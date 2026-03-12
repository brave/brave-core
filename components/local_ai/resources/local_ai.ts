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
} from 'gen/brave/components/local_ai/core/local_ai.mojom.m.js'

// Initialize connection to the browser-side LocalAIService
const localAIService = LocalAIService.getRemote()

class PassageEmbedderImpl implements PassageEmbedderInterface {
  async generateEmbeddings(_text: string) {
    // TODO(https://github.com/brave/brave-browser/issues/52722):
    // stub until model loading is wired up
    return { embedding: [] }
  }
}

class PassageEmbedderFactoryImpl implements PassageEmbedderFactoryInterface {
  receiver: PassageEmbedderFactoryReceiver

  constructor() {
    this.receiver = new PassageEmbedderFactoryReceiver(this)
  }

  bind(receiver: PassageEmbedderPendingReceiver) {
    const impl = new PassageEmbedderImpl()
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
