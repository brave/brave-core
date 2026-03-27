// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AIChatAPI } from './ai_chat_api'
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

  // Store the receiver so we can close it later
  const conversationUIReceiver = new Mojom.ConversationUIReceiver(
    conversationAPI.conversationUIObserver,
  )
  const conversationUIReceiverRemote =
    conversationUIReceiver.$.bindNewPipeAndPassRemote()

  if (id !== undefined) {
    aiChat.service.bindConversation(
      id,
      conversationHandlerReceiver,
      conversationUIReceiverRemote,
    )
  } else {
    aiChat.uiHandler.bindRelatedConversation(
      conversationHandlerReceiver,
      conversationUIReceiverRemote,
    )
  }

  // Optimistic update for conversation ID to prevent any jank when components
  // don't know what the conversation ID is for matching to an entry in the
  // conversation list.
  if (id) {
    conversationAPI.api.getState.update({ conversationUuid: id })
  }

  return {
    conversationHandler,
    close: () => {
      conversationAPI.close()
      conversationHandler.$.close()
      conversationUIReceiver.$.close()
    },
    api: conversationAPI.api,
  }
}

export function newConversation(aiChat: AIChatAPI['api']) {
  let conversationHandler = new Mojom.ConversationHandlerRemote()
  const conversationHandlerReceiver =
    conversationHandler.$.bindNewPipeAndPassReceiver()

  const conversationAPI = createConversationAPI(conversationHandler)

  // Store the receiver so we can close it later
  const conversationUIReceiver = new Mojom.ConversationUIReceiver(
    conversationAPI.conversationUIObserver,
  )
  const conversationUIReceiverRemote =
    conversationUIReceiver.$.bindNewPipeAndPassRemote()

  aiChat.uiHandler.newConversation(
    conversationHandlerReceiver,
    conversationUIReceiverRemote,
  )

  return {
    conversationHandler,
    close: () => {
      conversationAPI.close()
      conversationHandler.$.close()
      conversationUIReceiver.$.close()
    },
    api: conversationAPI.api,
  }
}

export type ConversationBindings = ReturnType<typeof newConversation>
