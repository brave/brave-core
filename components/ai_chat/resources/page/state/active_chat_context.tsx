// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useRoute } from '$web-common/useRoute'
import type * as BindConversation from '../api/bind_conversation'
import { useAIChat } from './ai_chat_context'

export const tabAssociatedChatId = 'tab'

export interface SelectedChatDetails
  extends Pick<
    BindConversation.ConversationBindings,
    'api' | 'conversationHandler'
  > {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  createNewConversation: () => void
  // TODO(https://github.com/brave/brave-browser/issues/48524): isTabAssociated
  // is not relevant for global side panel and causes UI side effects.
  isTabAssociated: boolean
}

export const ActiveChatContext = React.createContext<SelectedChatDetails>({
  selectedConversationId: undefined,
  updateSelectedConversationId: () => {},
  createNewConversation: () => {},
  isTabAssociated: false,
  api: undefined!,
  conversationHandler: undefined!,
})

export const updateSelectedConversation = (selectedId: string | undefined) => {
  window.history.pushState(null, '', `/${selectedId ?? ''}`)
}

type BindingProps = {
  bindConversation: typeof BindConversation.bindConversation
  newConversation: typeof BindConversation.newConversation
}

export function ActiveChatProviderFromUrl(
  props: React.PropsWithChildren<BindingProps>,
) {
  // Register the empty route, so we don't reload the page when navigating to '/'
  useRoute('/')

  const selectedConversationId = useRoute(`/{chatId}`)?.chatId
  return (
    <ActiveChatProvider
      selectedConversationId={selectedConversationId}
      updateSelectedConversationId={updateSelectedConversation}
      {...props}
    >
      {props.children}
    </ActiveChatProvider>
  )
}

export const useActiveChat = () => React.useContext(ActiveChatContext)

type ActiveChatContextProps = BindingProps & {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (selectedId: string | undefined) => void
}

function ActiveChatProvider(
  props: React.PropsWithChildren<ActiveChatContextProps>,
) {
  const aiChat = useAIChat()
  const [conversationAPI, setConversationAPI] =
    React.useState<BindConversation.ConversationBindings>()

  const [currentBoundConversationUuid, setCurrentBoundConversationUuid] =
    React.useState<string>()

  const details = React.useMemo<SelectedChatDetails>(
    () => ({
      // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
      ...conversationAPI!, // It's always got a value
      selectedConversationId: props.selectedConversationId,
      updateSelectedConversationId: props.updateSelectedConversationId,
      createNewConversation: () => {
        const bindings = props.newConversation(aiChat.api)
        if (bindings.conversationUuid) {
          setCurrentBoundConversationUuid(bindings.conversationUuid)
          updateSelectedConversation(bindings.conversationUuid)
        } else {
          bindings.conversationHandler
            .getConversationUuid()
            .then(({ conversationUuid }) => {
              setCurrentBoundConversationUuid(conversationUuid)
              updateSelectedConversation(conversationUuid)
            })
        }

        setConversationAPI(bindings)
      },
      isTabAssociated: props.selectedConversationId === tabAssociatedChatId,
    }),
    [
      props.selectedConversationId,
      props.updateSelectedConversationId,
      props.newConversation,
      setConversationAPI,
      conversationAPI,
    ],
  )

  // Only update conversation if we're on the tab associated conversation
  // and the event fires
  aiChat.api.useOnNewDefaultConversation(() => {
    if (props.selectedConversationId === tabAssociatedChatId) {
      // If the selected conversation is the tab associated one, we need to
      // bind to the new default conversation.
      setConversationAPI(props.bindConversation(aiChat.api, undefined))
    }
  }, [props.selectedConversationId, aiChat.api])

  // If the conversation ID changes explicitly to an ID or to blank,
  // we need to re-bind.
  React.useEffect(() => {
    // Handle creating a new conversation
    if (!props.selectedConversationId) {
      const bindings = props.newConversation(aiChat.api)
      if (bindings.conversationUuid) {
        setCurrentBoundConversationUuid(bindings.conversationUuid)
      } else {
        bindings.conversationHandler
          .getConversationUuid()
          .then(({ conversationUuid }) => {
            setCurrentBoundConversationUuid(conversationUuid)
          })
      }
      setConversationAPI(bindings)
      return
    }

    if (props.selectedConversationId === currentBoundConversationUuid) {
      return
    }

    // Select a specific conversation
    const bindings = props.bindConversation(
      aiChat.api,
      props.selectedConversationId === tabAssociatedChatId
        ? undefined
        : props.selectedConversationId,
    )
    if (bindings.conversationUuid) {
      setCurrentBoundConversationUuid(bindings.conversationUuid)
      updateSelectedConversation(bindings.conversationUuid)
    } else {
      bindings.conversationHandler
        .getConversationUuid()
        .then(({ conversationUuid }) => {
          setCurrentBoundConversationUuid(conversationUuid)
          updateSelectedConversation(conversationUuid)
        })
    }
    setConversationAPI(bindings)
  }, [aiChat.api, props.selectedConversationId])

  // Clean up bindings when not used anymore
  React.useEffect(() => {
    return () => {
      conversationAPI?.close()
    }
  }, [conversationAPI])

  const conversations = aiChat.api.useGetConversations().data

  // Handle the case where a non-existent chat has been selected:
  React.useEffect(() => {
    // We can't tell if an id is valid until we've loaded the list of
    // conversations.
    if (!aiChat.initialized) return

    // Special case the default conversation - it gets treated specially as
    // the chat is rebound as the tab navigates.
    if (props.selectedConversationId === tabAssociatedChatId) return
    if (!props.selectedConversationId) return
    if (conversations.find((c) => c.uuid === props.selectedConversationId))
      return

    // If this isn't a non-empty conversation, it could be an empty tab bound
    // conversation.
    let cancelled = false
    aiChat.api.actions.service
      .conversationExists(props.selectedConversationId)
      .then(({ exists }) => {
        if (cancelled) return
        if (exists) return
        props.updateSelectedConversationId(undefined)
      })

    return () => {
      cancelled = true
    }
  }, [conversations, props.selectedConversationId, aiChat.initialized])

  return (
    <ActiveChatContext.Provider value={details}>
      {conversationAPI && props.children}
    </ActiveChatContext.Provider>
  )
}
