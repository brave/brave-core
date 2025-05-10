// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useRoute } from '$web-common/useRoute'
import getAPI from '../api'
import { bindConversation, ConversationBindings } from '../api/bind_conversation'
import { useAIChat } from './ai_chat_context'

// Provides the bindings and anything needed to get data for and render
// a single conversation.
// This could be used to decide which conversation to render by checking
// the current UI's URL path, or it could be more simply provided
// for some feature requiring a specific conversation.
export interface SelectedChatDetails extends ConversationBindings {
  selectedConversationId: string | undefined
  isTabAssociated: boolean
  updateSelectedConversationId: (conversationId: string | undefined) => void
  createNewConversation: () => void,
}

export const ActiveChatContext = React.createContext<SelectedChatDetails>({
  selectedConversationId: undefined,
  isTabAssociated: false,
  updateSelectedConversationId: () => { },
  callbackRouter: undefined!,
  conversationHandler: undefined!,
  createNewConversation: () => { },
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
  const { initialized, visibleConversations, isStandalone } = useAIChat()
  const [conversationAPI, setConversationAPI] =
    React.useState<ConversationBindings>()

  const [currentBoundConversationUuid, setCurrentBoundConversationUuid] = React.useState<string>()

  React.useEffect(() => {
    if (!conversationAPI?.initialStatePromise) {
      setCurrentBoundConversationUuid(undefined)
      return
    }
    conversationAPI.initialStatePromise
      .then(({ conversationState }) => {
        setCurrentBoundConversationUuid(conversationState.conversationUuid)
      })
  }, [conversationAPI?.initialStatePromise])

  // Keep a reference to unsubscribe from the listener which follows
  // associated content navigation to re-bind to a new conversation so that
  // we can unsubscribe when changing to a specific conversation.
  const clearNewConversationListener = React.useRef(() => {})

  const bindToDefaultConversation = React.useCallback((createNewConversation: boolean) => {
    // Bind to an existing or new maybe-Tab-associated Conversation.
    // UI that isn't related to a Tab (e.g. standalone) will always
    // get a new conversation.
    setConversationAPI(bindConversation(undefined, createNewConversation))
    // The default conversation changes as the associated tab navigates, so
    // listen for changes. This event won't be fired for standalone UIs.
    const onNewDefaultConversationListenerId =
    getAPI().uiObserver.onNewDefaultConversation.addListener(() => {
      setConversationAPI(bindConversation(undefined))
    })
    return () => {
      getAPI().uiObserver.removeListener(onNewDefaultConversationListenerId)
    }
  }, [setConversationAPI])

  const createNewConversation = React.useCallback(() => {
    if (selectedConversationId) {
      updateSelectedConversationId(undefined)
    } else {
      // We're already in new or tab-default mode (URL path = /),
      // so we need to re-bind as URL change won't do anything.
      clearNewConversationListener.current()
      clearNewConversationListener.current = bindToDefaultConversation(true)
    }
  }, [bindToDefaultConversation])

  const details = React.useMemo(() => ({
    ...conversationAPI,
    selectedConversationId,
    isTabAssociated: !selectedConversationId && !isStandalone,
    updateSelectedConversationId,
    createNewConversation,
  }), [selectedConversationId, createNewConversation, updateSelectedConversationId, conversationAPI])

  React.useEffect(() => {
    // Setup bindings based on selectedConversationId (i.e. URL path)
    clearNewConversationListener.current()
    // Bind to a Conversation based on the selectedConversationId
    // New or default conversation when selectedConversationId is unspecified
    if (!selectedConversationId) {
      // Default or new conversation
      clearNewConversationListener.current = bindToDefaultConversation(false)
    } else {
      clearNewConversationListener.current = () => {}
      // Specific conversation
      // Avoid re-binding when URL updates to reflect a committed conversation
      if (currentBoundConversationUuid !== selectedConversationId) {
        setConversationAPI(bindConversation(selectedConversationId))
      }
    }
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
    // the chat is re-bound as the tab navigates.
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
