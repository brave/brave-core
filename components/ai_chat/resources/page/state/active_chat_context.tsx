// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../common/mojom'
import getAPI, * as API from '../api'
import { useRoute } from '$web-common/useRoute'
import { useAIChat } from './ai_chat_context'

export const tabAssociatedChatId = 'tab'

export interface SelectedChatDetails {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  conversationHandler: Mojom.ConversationHandlerRemote
  callbackRouter: Mojom.ConversationUICallbackRouter
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

  const isInitiallyTabAssociated = React.useMemo(() => {
    return (selectedConversationId === tabAssociatedChatId)
  }, [])

  const shouldCreateNewTabAssociatedConversation = React.useRef(false)

  const details = React.useMemo(() => ({
    ...conversationAPI,
    selectedConversationId,
    updateSelectedConversationId,
    createNewConversation: () => {
      if (isInitiallyTabAssociated) {
        // This is a bit ugly, because we want a single function to handle route changes and change the
        // content based on the URL (the useEffect below), but we need a way to specify that
        // we don't want the existing tab-default conversation, but we want a new tab-default conversation
        // to be created and disaplyed. We also don't want to call newConversation, or bind twice, so we shouldn't
        // call setConversationAPI here.
        shouldCreateNewTabAssociatedConversation.current = true
        location.href = `/${tabAssociatedChatId}`
      } else {
        location.href = '/'
      }
    },
    isTabAssociated: selectedConversationId === tabAssociatedChatId
  }), [selectedConversationId, updateSelectedConversationId, conversationAPI])

  React.useEffect(() => {
    // Bind to a Conversation based on the selectedConversationId

    // New conversation
    if (!selectedConversationId) {
      setConversationAPI(API.newConversation())
      return
    }

    // Bind to a maybe-Tab-associated conversation
    if (selectedConversationId === tabAssociatedChatId) {
      if (shouldCreateNewTabAssociatedConversation.current) {
        shouldCreateNewTabAssociatedConversation.current = false
        // Create a new Conversation, maybe associated with the active Tab
        setConversationAPI(API.newConversation())
      } else {
        // Bind to the existing maybe-Tab-associated Conversation
        setConversationAPI(API.bindConversation(undefined))
      }

      // The default conversation changes as the associated tab navigates, so
      // listen for changes.
      const onNewDefaultConversationListenerId =
        getAPI().uiObserver.onNewDefaultConversation.addListener(() => {
          setConversationAPI(API.bindConversation(undefined))
        })

      return () => {
        getAPI().uiObserver.removeListener(onNewDefaultConversationListenerId)
      }
    }

    // Specific conversation
    setConversationAPI(API.bindConversation(selectedConversationId))

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
