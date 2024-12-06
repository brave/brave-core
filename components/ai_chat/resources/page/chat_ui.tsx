/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '@brave/leo/tokens/css/variables.css'
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
  const iframeRef = React.useRef<HTMLIFrameElement>(null)
  const hasNotifiedLoaded = React.useRef(false)

  React.useEffect(() => {
    hasNotifiedLoaded.current = false
  }, [conversationContext.conversationUuid])

  React.useEffect(() => {
    const listener = (height: number) => {
      // Use the first height change to notify that the iframe has loaded,
      // in lieu of an actual "has rendered the conversation entries" event.
      if (!hasNotifiedLoaded.current && height > 0) {
        hasNotifiedLoaded.current = true
        props.onLoad()
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
  }, [props.onHeightChanged, props.onLoad])

  return (
    <iframe
      key={conversationContext.conversationUuid}
      src={"chrome-untrusted://leo-ai-conversation-entries/" + conversationContext.conversationUuid}
      ref={iframeRef}
    />
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
