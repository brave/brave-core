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

let promiseResolve_: (value: webllm.MLCEngine | PromiseLike<webllm.MLCEngine>) => void
const engineInitPromise = new Promise<webllm.MLCEngine>(resolve => promiseResolve_ = resolve)

Service.registerOnDeviceModelWorker(commandHandler.$.bindNewPipeAndPassRemote())
.then(async ({onDeviceModelName}) => {
    const engine = await webllm.CreateMLCEngine(onDeviceModelName, {
      initProgressCallback: (message) => {
        console.log('On Device Model Worker initProgressCallback:', message)
        Service.onDeviceModelWorkerStatusChanged(message.text, (message.progress >= 1))
      }
  })
  promiseResolve_(engine)
})

// TODO(petemill): allow each request to send the model name, and store a cache of engines


commandHandler.performRequest.addListener(
  async (
    messagesJson: string,
    responseHandler: mojom.OnDeviceModelResponseHandlerRemote
  ) => {
    console.log('On Device Model Worker received request:', {
      messagesJson,
      responseHandler
    })

    const engine = await engineInitPromise
    const chatParams: webllm.ChatCompletionRequestStreaming = JSON.parse(messagesJson)
    const reply = await engine.chat.completions.create(chatParams)
    if (chatParams.stream) {
      for await (const replyChunk of reply) {
        responseHandler.onPartialResponse(replyChunk.choices[0]?.delta.content || '')
      }
      responseHandler.onComplete('')
    } else {
      responseHandler.onComplete(await engine.getMessage())
    }

    return true
  }
)
