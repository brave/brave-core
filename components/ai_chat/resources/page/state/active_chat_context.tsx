// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useRoute } from '$web-common/useRoute'
import * as BindConversation from '../api/bind_conversation'
import useHasConversationStarted from '../hooks/useHasConversationStarted'
import { useAIChat } from './ai_chat_context'

export const tabAssociatedChatId = 'tab'

export interface SelectedChatDetails
  extends Pick<BindConversation.ConversationBindings, 'api'> {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  createNewConversation: () => void
  // TODO(https://github.com/brave/brave-browser/issues/48524): isTabAssociated
  // is not relevant for global side panel and causes UI side effects.
  isTabAssociated: boolean
}

/**
 * The purpose of ActiveChatContext is to decide what the current conversation
 * being displayed is and provide the bindings to that conversation.
 * One implementation of it is the ActiveChatProviderFromUrl which listens
 * to the Url and decides which conversation based on that.
 * Tests can provide their own version directly.
 */
export const ActiveChatContext = React.createContext<SelectedChatDetails>({
  selectedConversationId: undefined,
  updateSelectedConversationId: () => {},
  createNewConversation: () => {},
  isTabAssociated: false,

  // It's ok to stub undefined for now because we don't render children unless
  // we have a value. For convenience we don't need to have this as an optional
  // value.
  api: undefined!,
})

const updateSelectedConversation = (selectedId: string | undefined) => {
  window.history.pushState(null, '', `/${selectedId ?? ''}`)
}

export function ActiveChatProviderFromUrl(props: React.PropsWithChildren) {
  // Register the empty route, so we don't reload the page when navigating to '/'
  useRoute('/')

  const selectedConversationId = useRoute(`/{chatId}`)?.chatId
  return (
    <ActiveChatProvider
      selectedConversationId={selectedConversationId}
      updateSelectedConversationId={updateSelectedConversation}
    >
      {props.children}
    </ActiveChatProvider>
  )
}

export const useActiveChat = () => React.useContext(ActiveChatContext)

type ActiveChatContextProps = {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (selectedId: string | undefined) => void
}

function ActiveChatProvider({
  children,
  selectedConversationId,
  updateSelectedConversationId,
}: React.PropsWithChildren<ActiveChatContextProps>) {
  const aiChat = useAIChat()
  const [conversationAPI, setConversationAPI] =
    React.useState<BindConversation.ConversationBindings>()

  const details = React.useMemo<SelectedChatDetails>(
    () => ({
      ...conversationAPI!, // It's always got a value
      selectedConversationId,
      updateSelectedConversationId,
      createNewConversation: () => {
        // Special case for the tab-associated conversation mode
        if (selectedConversationId === tabAssociatedChatId) {
          // Simply bind a new conversation, this will make it associated
          // with the current tab, if applicable, via the UIHandler.
          setConversationAPI(BindConversation.newConversation(aiChat.api))
          return
        }

        // Otherwise, navigate to "/"
        updateSelectedConversation('')
      },
      isTabAssociated: selectedConversationId === tabAssociatedChatId,
    }),
    [selectedConversationId, updateSelectedConversationId, conversationAPI],
  )

  // Handle child frame requests we switch to new conversation
  aiChat.api.useRequestNewConversation(() => {
    details.createNewConversation()
  }, [aiChat.api])

  // Only update conversation if we're on the tab associated conversation
  // and the event fires
  aiChat.api.useOnNewDefaultConversation(() => {
    if (selectedConversationId === tabAssociatedChatId) {
      // If the selected conversation is the tab associated one, we need to
      // bind to the new default conversation.
      setConversationAPI(
        BindConversation.bindConversation(aiChat.api, undefined),
      )
    }
  }, [selectedConversationId, aiChat.api])

  // If the conversation ID changes explicitly to an ID or to blank,
  // we need to re-bind.
  React.useEffect(() => {
    // Handle creating a new conversation
    if (!selectedConversationId) {
      setConversationAPI(BindConversation.newConversation(aiChat.api))
      return
    }

    // getState is async fetched, so it's possible that it won't be populated
    // when we're checking for whether we need to re-bind to the conversation.
    // This is ok since this is an optimization to prevent re-binding to the
    // same conversation. Usually, if selectedConversationId changes, there's
    // been enough time to fetch getState.
    const currentlyBoundConversationUuid =
      conversationAPI?.api?.getState.current().conversationUuid
    if (currentlyBoundConversationUuid !== selectedConversationId) {
      // Select a specific conversation
      setConversationAPI(
        BindConversation.bindConversation(
          aiChat.api,
          selectedConversationId === tabAssociatedChatId
            ? undefined
            : selectedConversationId,
        ),
      )
    }
    // We only want the hook to re-fire when the selectedConversationId changes,
    // not again when the current conversationUuid does.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [aiChat.api, selectedConversationId])

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
    if (selectedConversationId === tabAssociatedChatId) return
    if (!selectedConversationId) return
    if (conversations.find((c) => c.uuid === selectedConversationId)) return

    // If this isn't a non-empty conversation, it could be an empty tab bound
    // conversation.
    let cancelled = false
    aiChat.api.service
      .conversationExists(selectedConversationId)
      .then(({ exists }) => {
        if (cancelled) return
        if (exists) return
        updateSelectedConversationId(undefined)
      })

    return () => {
      cancelled = true
    }
  }, [conversations, selectedConversationId, aiChat.initialized])

  return (
    <ActiveChatContext.Provider value={details}>
      {conversationAPI && (
        <>
          <URLUpdater {...details} />
          {children}
        </>
      )}
    </ActiveChatContext.Provider>
  )
}

function URLUpdater(props: SelectedChatDetails) {
  const conversationState = props.api.useGetStateData()

  // Update the location when the conversation has been started and we aren't
  // on a specific conversation URL yet, or we're not on a "special behavior"
  // conversation, i.e. the tab-associated conversation.
  const hasConversationStarted = useHasConversationStarted(
    conversationState.conversationUuid,
  )

  React.useEffect(() => {
    // Only change when we have an actual conversation worth re-visiting
    if (!hasConversationStarted) return

    // Stay on the tab-associated conversation so that we know to change
    // conversation automatically when the target tab navigates.
    if (props.selectedConversationId === tabAssociatedChatId) return

    // Don't need to do anything, the URL already represents the conversation
    if (conversationState.conversationUuid === props.selectedConversationId) {
      return
    }

    props.updateSelectedConversationId(conversationState.conversationUuid)
    // We don't want to re-run this effect on selectedConversationId change as
    // that would cause an infinite loop.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [hasConversationStarted, props.updateSelectedConversationId])

  return null
}
