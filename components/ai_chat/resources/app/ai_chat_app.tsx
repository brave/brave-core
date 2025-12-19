// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-components/app.global.scss'
import '$web-common/defaultTrustedTypesPolicy'
import * as Mojom from '../common/mojom'
import createAIChatAPI, { AIChatAPI } from '../page/api'
import {
  AIChatProvider,
  ConversationEntriesProps,
  useAIChat,
} from '../page/state/ai_chat_context'
import { ConversationProvider } from '../page/state/conversation_context'
import Main from '../page/components/main'
import FullScreen from '../page/components/full_page'
import Loading from '../page/components/loading'
import {
  ActiveChatProviderFromUrl,
  useActiveChat,
} from '../page/state/active_chat_context'
import Service from './api/ai_chat_service'
import Metrics from './api/metrics'
import BookmarksPageHandler from './api/bookmarks_page_handler'
import HistoryUIHandler from './api/history_ui_handler'
import UIHandler from './api/ui_handler'
import './load_time_data'
import type { ConversationBindings } from '../page/api/bind_conversation'
import createConversationAPI from '../page/api/conversation_api'
import { UntrustedConversationContextProvider } from '../untrusted_conversation_frame/untrusted_conversation_context'
import ConversationEntries from '../untrusted_conversation_frame/components/conversation_entries'
import createUntrustedConversationApi from '../untrusted_conversation_frame/api/untrusted_conversation_api'
import { Closable } from '$web-common/api'

setIconBasePath('/nala-icons')

console.log('AI Chat App: Starting up...')

// Create app versions of Interfaces
const service = new Service()
const metrics = new Metrics()
const bookmarksPageHandler = new BookmarksPageHandler()
const historyUIHandler = new HistoryUIHandler()
const uiHandler = new UIHandler()

// Feed to API factory
const aiChat = createAIChatAPI(
  service,
  uiHandler,
  bookmarksPageHandler,
  historyUIHandler,
  metrics,
)

// Wire up observers
service.bindObserverInterface(aiChat.serviceObserver).then(({ state }) => {
  aiChat.api.state.update(state)
})

uiHandler.setChatUIInterface(aiChat.chatUIObserver).then(({ isStandalone }) => {
  aiChat.api.isStandalone.update(isStandalone)
})

function bindNewConversation(aiChat: AIChatAPI['api']): ConversationBindings {
  const conversationHandler = service.newConversation()
  const conversationAPI = createConversationAPI(conversationHandler)

  conversationHandler.setObserver(conversationAPI.conversationUIObserver)

  return {
    conversationHandler,
    close: conversationAPI.close,
    api: conversationAPI.api,
  }
}

function bindConversation(
  aiChat: AIChatAPI['api'],
  id: string | undefined,
): ConversationBindings {
  if (id === undefined) {
    throw new Error('tab-default conversation not supported on this platform')
  }

  const conversationHandler = service.getConversationHandler(id)
  const conversationAPI = createConversationAPI(conversationHandler)

  conversationHandler.setObserver(conversationAPI.conversationUIObserver)

  return {
    conversationHandler,
    close: conversationAPI.close,
    api: conversationAPI.api,
  }
}

function App() {
  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  return (
    <AIChatProvider
      api={aiChat.api}
      conversationEntriesComponent={ConversationEntriesApp}
    >
      <ActiveChatProviderFromUrl
        bindConversation={bindConversation}
        newConversation={bindNewConversation}
      >
        <MainConversation />
      </ActiveChatProviderFromUrl>
    </AIChatProvider>
  )
}

function MainConversation() {
  // Get conversation based on the URL and set on this part of the tree
  const selectedConversationDetails = useActiveChat()
  return (
    <ConversationProvider {...selectedConversationDetails}>
      <Content />
    </ConversationProvider>
  )
}

function Content() {
  const aiChatContext = useAIChat()

  if (!aiChatContext.initialized || aiChatContext.isStandalone === undefined) {
    return <Loading />
  }

  if (!aiChatContext.isStandalone) {
    return <Main />
  }

  return <FullScreen />
}

function ConversationEntriesApp(props: ConversationEntriesProps) {
  // Main "frame" conversation bindings
  const activeChat = useActiveChat()

  const conversationEntriesApi = React.useMemo(() => {
    return createUntrustedConversationApi(
      activeChat.conversationHandler as unknown as Closable<Mojom.UntrustedConversationHandlerInterface>,
      uiHandler,
      aiChat.conversationEntriesFrameObserver as Closable<Mojom.ParentUIFrameInterface>,
    )
  }, [activeChat.conversationHandler])

  React.useEffect(() => {
    props.onIsContentReady(true)
  }, [conversationEntriesApi])

  return (
    <UntrustedConversationContextProvider api={conversationEntriesApi.api}>
      <ConversationEntries />
    </UntrustedConversationContextProvider>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
