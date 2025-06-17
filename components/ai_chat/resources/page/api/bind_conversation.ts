// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AIChatAPI } from './';
import * as Mojom from '../../common/mojom';
import createConversationAPI from './conversation_api';

//
// Binding helpers for conversations which are bound from either the browser UI handler
// or browser AIChat service
//

export function bindConversation(aiChat: AIChatAPI, id: string | undefined) {
  let conversationHandler = new Mojom.ConversationHandlerRemote()
  const conversationHandlerReceiver
    = conversationHandler.$.bindNewPipeAndPassReceiver()

  const conversationAPI = createConversationAPI(conversationHandler)

  if (id !== undefined) {
    aiChat.actions.service.bindConversation(
      id,
      conversationHandlerReceiver,
      conversationAPI.observer.$.bindNewPipeAndPassRemote()
    )
  } else {
    aiChat.actions.uiHandler.bindRelatedConversation(
      conversationHandlerReceiver,
      conversationAPI.observer.$.bindNewPipeAndPassRemote()
    )
  }
  return {
    conversationHandler,
    close: conversationAPI.close,
    api: conversationAPI.api,
  }
}

export function newConversation(aiChat: AIChatAPI) {
  let conversationHandler = new Mojom.ConversationHandlerRemote()
  const conversationHandlerReceiver
    = conversationHandler.$.bindNewPipeAndPassReceiver()

  const conversationAPI = createConversationAPI(conversationHandler)

  aiChat.actions.uiHandler.newConversation(
    conversationHandlerReceiver,
    conversationAPI.observer.$.bindNewPipeAndPassRemote()
  )

  return {
    conversationHandler,
    close: conversationAPI.close,
    api: conversationAPI.api,
  }
}

export type ConversationBindings = ReturnType<typeof newConversation>