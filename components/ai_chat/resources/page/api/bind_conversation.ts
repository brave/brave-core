// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AIChatAPI } from './'
import * as Mojom from '../../common/mojom'
import createConversationAPI from './conversation_api'

//
// Binding helpers for conversations which are bound from either the browser UI handler
// or browser AIChat service
//

export function bindConversation(
  aiChat: AIChatAPI['api'],
  id: string | undefined,
) {
  let conversationHandler = new Mojom.ConversationHandlerRemote()
  const conversationHandlerReceiver =
    conversationHandler.$.bindNewPipeAndPassReceiver()

  const conversationAPI = createConversationAPI(conversationHandler)

  const conversationUIReceiverRemote = new Mojom.ConversationUIReceiver(
    conversationAPI.conversationUIObserver,
  ).$.bindNewPipeAndPassRemote()

  if (id !== undefined) {
    aiChat.actions.service.bindConversation(
      id,
      conversationHandlerReceiver,
      conversationUIReceiverRemote,
    )
  } else {
    aiChat.actions.uiHandler.bindRelatedConversation(
      conversationHandlerReceiver,
      conversationUIReceiverRemote,
    )
  }
  return {
    conversationHandler,
    close: () => {
      conversationAPI.close()
      conversationHandler.$.close()
      conversationUIReceiverRemote.$.close()
    },
    api: conversationAPI.api,
  }
}

export function newConversation(aiChat: AIChatAPI['api']) {
  let conversationHandler = new Mojom.ConversationHandlerRemote()
  const conversationHandlerReceiver =
    conversationHandler.$.bindNewPipeAndPassReceiver()

  const conversationAPI = createConversationAPI(conversationHandler)

  const conversationUIReceiverRemote = new Mojom.ConversationUIReceiver(
    conversationAPI.conversationUIObserver,
  ).$.bindNewPipeAndPassRemote()

  aiChat.actions.uiHandler.newConversation(
    conversationHandlerReceiver,
    conversationUIReceiverRemote,
  )

  return {
    conversationHandler,
    close: conversationAPI.close,
    api: conversationAPI.api,
  }
}

export type ConversationBindings = ReturnType<typeof newConversation>
