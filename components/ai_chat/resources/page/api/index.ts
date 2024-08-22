/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'

class API {
  public Service: mojom.ServiceRemote
  public Observer: mojom.ServiceObserverCallbackRouter
  public UIHandler: mojom.AIChatUIHandlerRemote
  public UIObserver: mojom.ChatUICallbackRouter
  public isStandalone: boolean

  constructor() {
    // Connect to service
    this.Service = mojom.Service.getRemote()
    this.Observer = new mojom.ServiceObserverCallbackRouter()
    this.Service.bindObserver(this.Observer.$.bindNewPipeAndPassRemote())
    // Connect to platform UI handler
    this.UIHandler = mojom.AIChatUIHandler.getRemote()
    this.UIObserver = new mojom.ChatUICallbackRouter()
    this.UIObserver.setInitialData.addListener((isStandalone: boolean) => {
      this.isStandalone = isStandalone
    })
    this.UIHandler.setChatUI(this.UIObserver.$.bindNewPipeAndPassRemote())
  }
}

let apiInstance: API

export function setAPIForTesting(instance: API) {
  apiInstance = instance
}

export default function getAPI() {
  if (!apiInstance) {
    apiInstance = new API()
  }
  return apiInstance
}

export function bindConversation(id: string | undefined) {
  let conversationHandler: mojom.ConversationHandlerRemote =
    new mojom.ConversationHandlerRemote()
  let callbackRouter = new mojom.ConversationUICallbackRouter()

  if (id !== undefined) {
    getAPI().Service.bindConversation(
      id,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  } else {
    getAPI().UIHandler.bindRelatedConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  }
  return {
    conversationHandler,
    callbackRouter
  }
}

export function newConversation() {
  let conversationHandler: mojom.ConversationHandlerRemote =
      new mojom.ConversationHandlerRemote()
  let callbackRouter = new mojom.ConversationUICallbackRouter()
  getAPI().UIHandler.newConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote())
  return {
    conversationHandler,
    callbackRouter
  }
}
