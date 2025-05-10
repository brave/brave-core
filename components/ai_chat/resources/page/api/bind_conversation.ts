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

// There are three ways to bind to a conversation:
// 1. bindConversation(id)              - binds to an existing conversation with the given id.
// 2. bindConversation(undefined)       - binds to an existing conversation related to the current Tab.
//                                        A new conversation will be created if there is no related conversation.
//                                        A regular unassociated conversation will be created if this UI is not related
//                                        to a Tab.
// 3. bindConversation(undefined, true) - always binds to a new conversation,
//                                        the browser will decide whether it is related to an active Tab.

/**
 * Binds to an existing, related or new conversation as decided by the browser.
 * @param id Specific conversation uuid, or undefined for the conversation related to the current Tab if this
 *           UI is related to a Tab (e.g. opened from a Tab via the Sidebar or a menu item).
 * @param createNewConversation If true, a new conversation will be created, related to the currentTab if this
 *           UI is related to a Tab (e.g. opened from a Tab via the Sidebar or a menu item).
 * @returns Bindings directly to the conversation handler and conversation observer event emitter
 */
export function bindConversation(id: string | undefined, createNewConversation: boolean = false): ConversationBindings {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()
  let initialStatePromise: ConversationBindings['initialStatePromise']

  if (id !== undefined) {
    if (createNewConversation) {
      throw new Error('createNewConversation is not supported when id is defined')
    }
    initialStatePromise = getAPI().service.bindConversation(
      id,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  } else {
    initialStatePromise = getAPI().uiHandler.bindRelatedConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
      createNewConversation
    )
  }
  return {
    conversationHandler,
    callbackRouter,
    initialStatePromise
  }
}
