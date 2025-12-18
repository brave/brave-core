/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-components/app.global.scss'
import '$web-common/defaultTrustedTypesPolicy'
import * as Mojom from '../common/mojom'
import createAIChatAPI from './api'
import {
  AIChatProvider,
  ConversationEntriesProps,
  useAIChat,
} from './state/ai_chat_context'
import {
  ConversationProvider,
  useConversation,
  useConversationState,
} from './state/conversation_context'
import Main from './components/main'
import FullScreen from './components/full_page'
import Loading from './components/loading'
import {
  ActiveChatProviderFromUrl,
  useActiveChat,
} from './state/active_chat_context'

import '../common/strings'

// Perform any setup specific to this platform

setIconBasePath('chrome://resources/brave-icons')

// Create global mojo connections
const serviceRemote = Mojom.Service.getRemote()
const metricsRemote = new Mojom.MetricsRemote()
serviceRemote.bindMetrics(metricsRemote.$.bindNewPipeAndPassReceiver())
const uiHandlerRemote = Mojom.AIChatUIHandler.getRemote()
const tabTrackerServiceRemote = Mojom.TabTrackerService.getRemote()

const aiChat = createAIChatAPI(
  serviceRemote,
  uiHandlerRemote,
  Mojom.BookmarksPageHandler.getRemote(),
  Mojom.HistoryUIHandler.getRemote(),
  metricsRemote,
)

// Bind mojo receivers to the appropriate observers, routing events.

const serviceObserverReceiver = new Mojom.ServiceObserverReceiver(
  aiChat.serviceObserver,
)
serviceRemote
  .bindObserver(serviceObserverReceiver.$.bindNewPipeAndPassRemote())
  .then(({ state }) => {
    aiChat.api.state.update(state)
  })

const chatUIReceiver = new Mojom.ChatUIReceiver(aiChat.chatUIObserver)
uiHandlerRemote
  .setChatUI(chatUIReceiver.$.bindNewPipeAndPassRemote())
  .then(({ isStandalone }) => {
    aiChat.api.isStandalone.update(isStandalone)
  })
const tabDataObserverReceiver = new Mojom.TabDataObserverReceiver(
  aiChat.tabDataObserver,
)
tabTrackerServiceRemote.addObserver(
  tabDataObserverReceiver.$.bindNewPipeAndPassRemote(),
)

// Receive child frame interface
aiChat.api.subscribeToOnChildFrameBound((parentPageReceiver) => {
  new Mojom.ParentUIFrameReceiver(
    aiChat.conversationEntriesFrameObserver,
  ).$.bindHandle(parentPageReceiver.handle)
})

function App() {
  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  return (
    <AIChatProvider
      api={aiChat.api}
      conversationEntriesComponent={ConversationEntries}
    >
      <ActiveChatProviderFromUrl>
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

function ConversationEntries(props: ConversationEntriesProps) {
  const aiChatContext = useAIChat()

  const { api: conversationApi } = useConversation()
  const state = useConversationState()

  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const hasNotifiedContentReady = React.useRef(false)
  const [hasLoaded, setHasLoaded] = React.useState(false)

  // Notify onIsContentReady when
  // - iframe increases in height after a conversation change OR
  // - iframe is loaded AND iframe conversation length is 0 after a conversation change

  // Reset when conversation changes
  React.useEffect(() => {
    setHasLoaded(false)
    props.onIsContentReady(false)
    hasNotifiedContentReady.current = false
    if (iframeRef.current) {
      iframeRef.current.style.height = '0px'
    }
  }, [state.conversationUuid, props.onIsContentReady])

  const conversationHasEntries =
    !!conversationApi.useGetConversationHistory().getConversationHistoryData
      .length

  // Mark that iframe has loaded if there're no conversation entries,
  // since we won't get ChildHeightChanged notification in that case.
  React.useEffect(() => {
    // conversationUuid populated is a sign that data has been fetched
    if (
      !hasNotifiedContentReady.current
      && state.conversationUuid
      && !conversationHasEntries
      && hasLoaded
    ) {
      hasNotifiedContentReady.current = true
      props.onIsContentReady(true)
    }
  }, [state.conversationUuid, conversationHasEntries, hasLoaded])

  // When height of frame content changes, update the iframe height
  aiChatContext.api.useChildHeightChanged(
    (height) => {
      // Use the first height change to notify that the iframe has rendered,
      // in lieu of an actual "has rendered the conversation entries" event
      // which, if we get any bugs with this and need to add complexity, might
      // be simpler to implement explicitly, from child -> parent.
      if (!hasNotifiedContentReady.current && height) {
        hasNotifiedContentReady.current = true
        props.onIsContentReady(true)
      }
      if (height && iframeRef.current) {
        // Additional height is added here to address the issue where the
        // button menu's get cut off when the conversation is short since
        // they cant be rendered outside of the iframe.
        // See https://github.com/brave/brave-browser/issues/46042
        const additionalHeight = Math.max(0, 600 - height)
        document.body.style.setProperty(
          '--iframe-additional-margin-for-menus',
          additionalHeight + 'px',
        )
        iframeRef.current.style.height = height + additionalHeight + 'px'
      }
    },
    [props.onIsContentReady],
  )

  aiChatContext.api.useRegenerateAnswerMenuIsOpen((isOpen) => {
    // Set the iframe position to relative when the regenerate
    // answer menu is open. Otherwise the menu can sometimes be
    // overlapped by the Suggested question buttons.
    document.body.style.setProperty(
      '--iframe-position-for-menus',
      isOpen ? 'relative' : 'unset',
    )
  })

  return (
    <iframe
      sandbox='allow-scripts allow-same-origin allow-modals allow-forms allow-popups allow-popups-to-escape-sandbox'
      allow='clipboard-write'
      src={
        'chrome-untrusted://leo-ai-conversation-entries/'
        + state.conversationUuid
      }
      ref={iframeRef}
      data-testid='conversation-entries-iframe'
      onLoad={() => setHasLoaded(true)}
    />
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
