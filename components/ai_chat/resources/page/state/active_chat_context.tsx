// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as AIChat from '../api'
import { useRoute } from '$web-common/useRoute'
import { useAIChat } from './ai_chat_context'

export const tabAssociatedChatId = 'tab'

export interface SelectedChatDetails {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  conversationHandler: AIChat.ConversationHandlerRemote
  callbackRouter: AIChat.ConversationUICallbackRouter
  createNewConversation: () => void,
  isTabAssociated: boolean
}

export const ActiveChatContext = React.createContext<SelectedChatDetails>({
  selectedConversationId: undefined,
  updateSelectedConversationId: () => { },
  callbackRouter: undefined!,
  conversationHandler: undefined!,
  createNewConversation: () => { },
  isTabAssociated: false,
})


const updateSelectedConversation = (selectedId: string | undefined) => {
  window.location.href = `/${selectedId ?? ''}`
}

export function ActiveChatProviderFromUrl(props: React.PropsWithChildren) {
  // Register the empty route, so we don't reload the page when navigating to '/'
  useRoute('/')

  const selectedConversationId = useRoute(`/{chatId}`)?.chatId
  return <ActiveChatProvider selectedConversationId={selectedConversationId} updateSelectedConversationId={updateSelectedConversation}>
    {props.children}
  </ActiveChatProvider>
}

export const useActiveChat = () => React.useContext(ActiveChatContext)

function ActiveChatProvider({ children, selectedConversationId, updateSelectedConversationId }: React.PropsWithChildren<{
  selectedConversationId: string | undefined
  updateSelectedConversationId: (selectedId: string | undefined) => void,
}>) {
  const { initialized, visibleConversations } = useAIChat()
  const [conversationAPI, setConversationAPI] =
    React.useState<Pick<SelectedChatDetails, 'callbackRouter' | 'conversationHandler'>>()

  const details = React.useMemo(() => ({
    ...conversationAPI,
    selectedConversationId,
    updateSelectedConversationId,
    createNewConversation: () => {
      setConversationAPI(AIChat.newConversation())
    },
    isTabAssociated: selectedConversationId === tabAssociatedChatId
  }), [selectedConversationId, updateSelectedConversationId, conversationAPI])

  React.useEffect(() => {
    // Handle creating a new conversation
    if (!selectedConversationId) {
      setConversationAPI(AIChat.newConversation())
      return
    }

    // Select a specific conversation
    setConversationAPI(AIChat.bindConversation(selectedConversationId === tabAssociatedChatId
      ? undefined
      : selectedConversationId))

    // The default conversation changes as the associated tab navigates, so
    // listen for changes.
    if (selectedConversationId === tabAssociatedChatId) {
      const onNewDefaultConversationListenerId =
        getAPI().uiObserver.onNewDefaultConversation.addListener(() => {
          setConversationAPI(AIChat.bindConversation(undefined))
        })

      return () => {
        getAPI().uiObserver.removeListener(onNewDefaultConversationListenerId)
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
    if (!selectedConversationId) return
    if (visibleConversations.find(c => c.uuid === selectedConversationId)) return

    // If this isn't a visible conversation, it could be an empty tab bound
    // conversation.
    let cancelled = false
    getAPI().service.conversationExists(selectedConversationId).then(({ exists }) => {
      if (cancelled) return
      if (exists) return
      updateSelectedConversationId(undefined)
    })

    return () => {
      cancelled = true
    }
  }, [visibleConversations, selectedConversationId, initialized])

  return <ActiveChatContext.Provider value={details as any}>
    {conversationAPI && children}
  </ActiveChatContext.Provider>
}
