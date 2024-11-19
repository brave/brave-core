// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as API from '../api'
import { useRoute } from '$web-common/useRoute'
import { useAIChat } from './ai_chat_context'

export const tabAssociatedChatId = 'tab'

export interface SelectedChatDetails {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  conversationHandler: API.ConversationHandlerRemote
  callbackRouter: API.ConversationUICallbackRouter
}


const updateSelectedConversation = (selectedId: string | undefined) => {
  window.location.href = `/${selectedId ?? ''}`
}

// Note: We render children using the RenderProps pattern, so we can provide
// different strategies for deciding the ActiveConversation (such as from the
// URL). This will be useful when we move the content into an iframe.
// https://github.com/brave/brave-core/pull/26050#issuecomment-2418561971
type Children = (details: SelectedChatDetails) => JSX.Element
export function ActiveChatProviderFromUrl(props: {
  children: Children
}) {
  // Register the empty route, so we don't reload the page when navigating to '/'
  useRoute('/')

  const selectedConversationId = useRoute(`/{chatId}`)?.chatId
  return <ActiveChatProvider selectedConversationId={selectedConversationId} updateSelectedConversationId={updateSelectedConversation}>
    {props.children}
  </ActiveChatProvider>
}

function ActiveChatProvider({ children, selectedConversationId, updateSelectedConversationId }: {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (selectedId: string | undefined) => void,
  children: Children
}) {
  const { visibleConversations, initialized } = useAIChat()
  const [conversationAPI, setConversationAPI] =
    React.useState<Pick<SelectedChatDetails, 'callbackRouter' | 'conversationHandler'>>()

  const details = React.useMemo(() => ({
    ...conversationAPI,
    selectedConversationId,
    updateSelectedConversationId,
  }), [selectedConversationId, updateSelectedConversationId, conversationAPI])

  React.useEffect(() => {
    // Handle creating a new conversation
    if (!selectedConversationId) {
      setConversationAPI(API.newConversation())
      return
    }

    // Select a specific conversation
    setConversationAPI(API.bindConversation(selectedConversationId === tabAssociatedChatId
      ? undefined
      : selectedConversationId))

    // The default conversation changes as the associated tab navigates, so
    // listen for changes.
    if (selectedConversationId === tabAssociatedChatId) {
      const onNewDefaultConversationListenerId =
        getAPI().UIObserver.onNewDefaultConversation.addListener(() => {
          setConversationAPI(API.bindConversation(undefined))
        })

      return () => {
        getAPI().UIObserver.removeListener(onNewDefaultConversationListenerId)
      }
    }

    // Satisfy linter
    return undefined
  }, [selectedConversationId])

  // Clean up bindings when not used anymore
  React.useEffect(() => {
    return () => {
      conversationAPI?.callbackRouter.$.close()
      conversationAPI?.conversationHandler.$.close()
    }
  }, [conversationAPI])

  // Handle the case where a non-existent chat has been selected:
  React.useEffect(() => {
    // We can't tell if an id is valid until we've loaded the list of visible
    // conversations.
    if (!initialized) return

    // Special case the default conversation - it gets treated specially as
    // the chat is rebound as the tab navigates.
    if (selectedConversationId === tabAssociatedChatId) return

    if (selectedConversationId && !visibleConversations.find(c => c.uuid === selectedConversationId)) {
      updateSelectedConversationId(undefined)
    }
  }, [visibleConversations, selectedConversationId, initialized])

  return conversationAPI
    ? children(details as any)
    : null
}
