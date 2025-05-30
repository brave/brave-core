// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import getAPI from './'

export interface ConversationBindings {
  conversationHandler: Mojom.ConversationHandlerRemote
  callbackRouter: Mojom.ConversationUICallbackRouter
  initialStatePromise?: ReturnType<Mojom.ServiceRemote['bindConversation']>
}

// Create some keywords as alternatives to string UUID for a conversation identifier.
// Use symbols instead of strings to ensure they are not confused with actual UUIDs,
// and cause a compile error when they are modified.
export const ConversationType = {
  New: Symbol('ConversationTypeNew'),
  Related: Symbol('ConversationTypeRelated'),
}
type ConversationType = typeof ConversationType[keyof typeof ConversationType]

/**
 * Binds to an existing, related or new conversation as decided by the browser.
 * @param uuidOrType Specific conversation uuid, or type of conversation to create or find. Related conversation may be
 *           related to the current Tab if this UI is related to a Tab
 *           (e.g. opened from a Tab via the Sidebar or a menu item). New conversation will always create a new
 *          conversation, but it may be related to the current Tab if this UI is related to a Tab.
 * @returns Bindings directly to the conversation handler and conversation observer event emitter
 */
export function bindConversation(uuidOrType:  ConversationType | string): ConversationBindings {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()
  let initialStatePromise: ConversationBindings['initialStatePromise']

  if (typeof uuidOrType === 'string') {
    initialStatePromise = getAPI().service.bindConversation(
      uuidOrType,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  } else {
    initialStatePromise = getAPI().uiHandler.bindDefaultConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
      uuidOrType === ConversationType.New
    )
  }
  return {
    conversationHandler,
    callbackRouter,
    initialStatePromise
  }
}
