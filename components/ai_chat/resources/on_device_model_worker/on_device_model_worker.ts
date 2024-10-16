// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'

console.log('On Device Model Worker registering with Service...')

// Establish connection with Service
const Service: mojom.ServiceRemote = mojom.Service.getRemote()

const commandHandler: mojom.OnDeviceModelWorkerCallbackRouter =
  new mojom.OnDeviceModelWorkerCallbackRouter()

Service.registerOnDeviceModelWorker(commandHandler.$.bindNewPipeAndPassRemote())

commandHandler.performRequest.addListener(
  async (
    messagesJson: string,
    responseHandler: mojom.OnDeviceModelResponseHandlerRemote,
    callback: (is_success: boolean) => unknown
  ) => {
    console.log('On Device Model Worker received request:', {
      messagesJson,
      responseHandler,
      callback
    })
    // Perform request
    responseHandler.onPartialResponse('hello')
    responseHandler.onPartialResponse(' from')
    await new Promise((resolve) =>
      setTimeout(() => {
        responseHandler.onPartialResponse(' the page!')
        resolve(true)
      }, 1000)
    )
    responseHandler.onComplete('')
    return true
  }
)
