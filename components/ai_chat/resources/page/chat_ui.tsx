/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-common/defaultTrustedTypesPolicy'
import * as Mojom from '../common/mojom'
import bindWebUiServices from './api/bind_webui_services'
import useUpdateDocumentTitle from './hooks/useUpdateDocumentTitle'
import {
  AIChatProvider,
  ConversationEntriesProps,
  useAIChat,
} from './state/ai_chat_context'
import {
  ConversationProvider,
  useConversationState,
} from './state/conversation_context'
import Main from './components/main'
import FullScreen from './components/full_page'
import Loading from './components/loading'
import {
  ActiveChatProviderFromUrl,
  useActiveChat,
} from './state/active_chat_context'
import styles from './chat_ui.module.scss'

import '../common/strings'
// <if expr="is_ios">
import { useIOSOneTapFix } from '../common/useIOSOneTapFix'
// </if>

// Perform any setup specific to this platform

setIconBasePath('chrome://resources/brave-icons')

// Create global mojo connections
const aiChat = bindWebUiServices()

// Receive child frame interface
aiChat.api.subscribeToOnChildFrameBound((parentPageReceiver) => {
  new Mojom.ParentUIFrameReceiver(
    aiChat.conversationEntriesFrameObserver,
  ).$.bindHandle(parentPageReceiver.handle)
})

function App() {
  // <if expr="is_ios">
  useIOSOneTapFix()

  // When the iframe calls dismissMenus() (user tapped/clicked there), trigger a
  // click in the parent so Leo's clickOutside closes any open menus.
  aiChat.api.useDismissMenus(() => {
    document.body.click()
  })
  // </if>

  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  return (
    <AIChatProvider
      api={aiChat.api}
      conversationEntriesComponent={ConversationEntries}
    >
      <ContentWithConversationContext />
    </AIChatProvider>
  )
}

function ContentWithConversationContext() {
  const aiChatContext = useAIChat()

  if (!aiChatContext.initialized || aiChatContext.isStandalone === undefined) {
    // Don't load the ActiveChatProvider until the database is initialized.
    // Otherwise it will always create a new conversation instead of loading one
    // which will cause a new conversation to be created and 'initialized' set
    // to true prematurely.
    return <Loading />
  }

  return (
    <ActiveChatProviderFromUrl>
      <MainConversation />
    </ActiveChatProviderFromUrl>
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

  useUpdateDocumentTitle()

  if (!aiChatContext.isStandalone) {
    return <Main />
  }

  return <FullScreen />
}

function ConversationEntries(props: ConversationEntriesProps) {
  const state = useConversationState()

  const [iframeSrc, setIframeSrc] = React.useState<string>()

  React.useEffect(() => {
    // The state conversationUuid can bounce from a valid value to a null
    // value whilst the conversationUuid for a newly-bound conversation is
    // fetched. Only update the Src once this settles.
    if (!state.conversationUuid) {
      return
    }
    setIframeSrc(
      `chrome-untrusted://leo-ai-conversation-entries/${state.conversationUuid}`,
    )
  }, [state.conversationUuid])

  return (
    <div className={props.className}>
      {iframeSrc && (
        <iframe
          data-testid='conversation-entries-iframe'
          className={styles.conversationEntriesFrame}
          sandbox='allow-scripts allow-same-origin allow-modals allow-forms allow-popups allow-popups-to-escape-sandbox'
          allow='clipboard-write'
          src={iframeSrc}
        />
      )}
    </div>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
