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

  // Set initial state
  // Emit the event instead of directly updating the store so that any custom
  // handling (e.g. model filtering) happens and we don't need to duplicate
  // here.
  conversationAPI.api.emitEvent('onEntriesUIStateChanged', [
    conversationEntriesState,
  ])

  // Set up document height communication with parent frame
  const sendDocumentHeight = () => {
    parentUIFrame.childHeightChanged(document.body.clientHeight)
  }

  window.addEventListener('resize', sendDocumentHeight)
  new ResizeObserver(sendDocumentHeight).observe(document.body)
  sendDocumentHeight()

  return {
    api: conversationAPI.api,
    close: () => {
      conversationAPI.close()
      conversationUIReceiver.$.close()
      uiReceiver.$.close()
    },
  }
}

export type BoundUntrustedConversation = Awaited<
  ReturnType<typeof bindUntrustedConversation>
>
