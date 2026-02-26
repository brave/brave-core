// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import createUntrustedConversationApi from './untrusted_conversation_api'

/**
 * Creates WebUi bindings to mojo interfaces for the active conversation with
 * Conversation uuid found on the URL path. Returns the
 * createUntrustedConversationApi result.
 */
export async function bindUntrustedConversation() {
  const conversationHandler = new Mojom.UntrustedConversationHandlerRemote()
  const uiHandler = Mojom.UntrustedUIHandler.getRemote()
  const parentUIFrame = new Mojom.ParentUIFrameRemote()
  // Service is bound directly to the WebUI via the interface broker
  const service = Mojom.UntrustedService.getRemote()

  // This frame gets the conversation ID from the URL
  const conversationId = window.location.pathname.split('/').pop() || ''

  uiHandler.bindConversationHandler(
    conversationId,
    conversationHandler.$.bindNewPipeAndPassReceiver(),
  )

  uiHandler.bindParentPage(parentUIFrame.$.bindNewPipeAndPassReceiver())

  const conversationAPI = createUntrustedConversationApi(
    conversationHandler,
    uiHandler,
    parentUIFrame,
    service,
  )

  const uiReceiver = new Mojom.UntrustedUIReceiver(conversationAPI.uiObserver)
  uiHandler.bindUntrustedUI(uiReceiver.$.bindNewPipeAndPassRemote())

  const conversationUIReceiver = new Mojom.UntrustedConversationUIReceiver(
    conversationAPI.conversationObserver,
  )
  const { conversationEntriesState } =
    await conversationHandler.bindUntrustedConversationUI(
      conversationUIReceiver.$.bindNewPipeAndPassRemote(),
    )

  // Bind the service observer and get initial service state
  const serviceObserverReceiver = new Mojom.UntrustedServiceObserverReceiver(
    conversationAPI.serviceObserver,
  )
  const { state: serviceState } = await service.bindObserver(
    serviceObserverReceiver.$.bindNewPipeAndPassRemote(),
  )

  // Set initial state
  conversationAPI.conversationObserver.onEntriesUIStateChanged(
    conversationEntriesState,
  )
  // Set initial service state
  conversationAPI.api.emitEvent('onStateChanged', [serviceState])

  return {
    api: conversationAPI.api,
    close: () => {
      conversationAPI.close()
      conversationUIReceiver.$.close()
      uiReceiver.$.close()
      serviceObserverReceiver.$.close()
    },
  }
}

export type BoundUntrustedConversation = Awaited<
  ReturnType<typeof bindUntrustedConversation>
>
