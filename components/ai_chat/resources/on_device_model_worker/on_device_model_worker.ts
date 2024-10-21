// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as webllm from '@mlc-ai/web-llm'
import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'

console.log('On Device Model Worker registering with Service...')

// Establish connection with Service
const Service: mojom.ServiceRemote = mojom.Service.getRemote()

const commandHandler: mojom.OnDeviceModelWorkerCallbackRouter =
  new mojom.OnDeviceModelWorkerCallbackRouter()

Service.registerOnDeviceModelWorker(commandHandler.$.bindNewPipeAndPassRemote())
.then(({ args }) => {
  console.debug('got args from service', args)
})

// Run multiple model engines in parallel
const engines: Map<string, { engine: Promise<webllm.MLCEngine>, timerId: number }> = new Map()

function getEngine(modelName: string): Promise<webllm.MLCEngine> {
  if (!engines.has(modelName)) {
    engines.set(modelName, { engine: new Promise(async resolve => {
      const engine = new webllm.MLCEngine({initProgressCallback: (message) => {
        console.log('On Device Model Worker initProgressCallback:', message)
        Service.onDeviceModelWorkerStatusChanged(modelName, message.text, (message.progress >= 1))
      }})
      await engine.reload(modelName)
      resolve(engine)
    }), timerId: 0})
  }
  const engineInfo = engines.get(modelName)!
  clearTimeout(engineInfo.timerId)
  return engineInfo.engine
}

// Allow engines to be unloaded after a period of inactivity
function doneWithEngine(modelKey: string) {
  const engineInfo = engines.get(modelKey)
  if (engineInfo) {
    clearTimeout(engineInfo.timerId)
    engineInfo.timerId = window.setTimeout(async () => {
      const engine = await engineInfo.engine
      engines.delete(modelKey)
      console.debug('Unloading engine', modelKey)
      await engine.unload()
      console.debug('Engine unloaded', modelKey)
    }, 10000) // 10 seconds
  }
}

commandHandler.performRequest.addListener(
  async (
    modelKey: string,
    messagesJson: string,
    responseHandler: mojom.OnDeviceModelResponseHandlerRemote
  ) => {
  console.log('On Device Model Worker received request:', {
    messagesJson,
    responseHandler
  })
  try {
    const engine = await getEngine(modelKey)
    const chatParams: webllm.ChatCompletionRequest = JSON.parse(messagesJson)
    if (chatParams.stream) {
      const reply = await engine.chat.completions.create(chatParams)
      // Streaming response
      for await (const replyChunk of reply) {
        responseHandler.onPartialResponse(replyChunk.choices[0]?.delta.content || '')
      }
      responseHandler.onComplete('')
    } else {
      await engine.chat.completions.create(chatParams)
      // Non-streaming response
      responseHandler.onComplete(await engine.getMessage())
    }
    return true
  } catch (e) {
    console.error(e)
    return false
  } finally {
    // Release engine
    doneWithEngine(modelKey)
  }
})
