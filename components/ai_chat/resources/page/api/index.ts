/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { AIChatAPI } from './ai_chat_api'
import * as Mojom from '../../common/mojom'

export function bindConversation(
  aiChat: AIChatAPI['api'],
  id: string | undefined,
) {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()

  if (id !== undefined) {
    aiChat.service.bindConversation(
      id,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
    )
  } else {
    aiChat.uiHandler.bindRelatedConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
    )
  }
  return {
    conversationHandler,
    callbackRouter,
  }
}

export function newConversation(aiChat: AIChatAPI['api']) {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()
  aiChat.uiHandler.newConversation(
    conversationHandler.$.bindNewPipeAndPassReceiver(),
    callbackRouter.$.bindNewPipeAndPassRemote(),
  )
  return {
    conversationHandler,
    callbackRouter,
  }
}

export type ConversationBindings = ReturnType<typeof bindConversation>
