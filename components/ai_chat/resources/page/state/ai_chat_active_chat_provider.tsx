// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as API from '../api'
import { tabAssociatedChatId, useSelectedConversation } from '../routes'

export interface SelectedChatDetails {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (conversationId: string | undefined) => void
  conversationHandler: API.ConversationHandlerRemote
  callbackRouter: API.ConversationUICallbackRouter
}

type Children = (details: SelectedChatDetails) => JSX.Element

const updateSelectedConversation = (selectedId: string | undefined) => {
  window.location.href = `/${selectedId ?? ''}`
}

export function ActiveChatProviderFromUrl(props: {
  children: Children
}) {
  const selectedConversationId = useSelectedConversation()
  return <ActiveChatProvider selectedConversationId={selectedConversationId} updateSelectedConversationId={updateSelectedConversation}>
    {props.children}
  </ActiveChatProvider>
}

function ActiveChatProvider({ children, selectedConversationId, updateSelectedConversationId }: {
  selectedConversationId: string | undefined
  updateSelectedConversationId: (selectedId: string | undefined) => void,
  children: Children
}) {
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

  return conversationAPI
    ? children(details as any)
    : null
}
