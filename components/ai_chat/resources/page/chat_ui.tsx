/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-components/app.global.scss'
import '$web-common/defaultTrustedTypesPolicy'
import getAPI from './api'
import { AIChatContextProvider, ConversationEntriesProps, useAIChat } from './state/ai_chat_context'
import {
  ConversationContextProvider,
  useConversation
} from './state/conversation_context'
import Main from './components/main'
import FullScreen from './components/full_page'
import Loading from './components/loading'
import { ActiveChatProviderFromUrl } from './state/active_chat_context'

setIconBasePath('chrome://resources/brave-icons')

// Make sure we're fetching data as early as possible
const api = getAPI()

function App() {
  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  return (
    <AIChatContextProvider conversationEntriesComponent={ConversationEntries}>
      <ActiveChatProviderFromUrl>
        <ConversationContextProvider>
            <Content />
        </ConversationContextProvider>
      </ActiveChatProviderFromUrl>
    </AIChatContextProvider>
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
  const conversationContext = useConversation()
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
  }, [conversationContext.conversationUuid, props.onIsContentReady])

  // The iframe has loaded if there're no conversation entries,
  // it will never grow until a user action happens.
  React.useEffect(() => {
    // conversationUuid populated is a sign that data has been fetched
    if (!hasNotifiedContentReady.current && conversationContext.conversationUuid &&
        !conversationContext.conversationHistory.length && hasLoaded) {
      hasNotifiedContentReady.current = true
      props.onIsContentReady(true)
    }
  }, [
    conversationContext.conversationUuid,
    conversationContext.conversationHistory.length,
    hasLoaded
  ])

  React.useEffect(() => {
    const listener = (height: number) => {
      // Use the first height change to notify that the iframe has rendered,
      // in lieu of an actual "has rendered the conversation entries" event
      // which, if we get any bugs with this and need to add complexity, might
      // be simpler to implement excplicitly, from child -> parent.
      if (!hasNotifiedContentReady.current && height > 0) {
        hasNotifiedContentReady.current = true
        props.onIsContentReady(true)
      }
      if (iframeRef.current) {
        iframeRef.current.style.height = height + 'px'
        props.onHeightChanged()
      }
    }
    const id = api.conversationEntriesFrameObserver.childHeightChanged.addListener(listener)

    return () => {
      api.conversationEntriesFrameObserver.removeListener(id)
    }
  }, [props.onHeightChanged, props.onIsContentReady])

  return (
    <iframe
      sandbox='allow-scripts allow-same-origin'
      allow='clipboard-write'
      src={'chrome-untrusted://leo-ai-conversation-entries/' + conversationContext.conversationUuid}
      ref={iframeRef}
      onLoad={() => setHasLoaded(true)}
    />
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
