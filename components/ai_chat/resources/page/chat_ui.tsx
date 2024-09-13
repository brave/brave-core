/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { initLocale } from 'brave-ui'
import { setIconBasePath } from '@brave/leo/react/icon'

import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'

import '$web-common/defaultTrustedTypesPolicy'
import { loadTimeData } from '$web-common/loadTimeData'
import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'
import getAPI, * as API from './api'
import { AIChatContextProvider } from './state/ai_chat_context'
import Main from './components/main'
import {
  ConversationContextProps,
  ConversationContextProvider
} from './state/conversation_context'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App() {
  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  const [selectedConversationUuid, setSelectedConversationUuid] = React.useState<
    string | undefined
  >()

  const [conversationAPI, setConversationAPI] =
    React.useState<ConversationContextProps>()

  // A token so that we can re-bind to a new default conversation when
  // the associated content navigates
  const [defaultConversationToken, setDefaultConversationToken] =
    React.useState(new Date().getTime())

  const handleSelectConversationUuid = (id: string | undefined) => {
    console.log('select conversation', id)
    setConversationAPI(API.bindConversation(id))
    setSelectedConversationUuid(id)
  }

  // Start off with default conversation and if the target content
  // navigates then show the new conversation, only if we're still
  // on the default conversation.
  React.useEffect(() => {
    if (!selectedConversationUuid) {
      handleSelectConversationUuid(undefined)
    }
  }, [defaultConversationToken])

  // Clean up bindings when not used anymore
  React.useEffect(() => {
    return () => {
      conversationAPI?.callbackRouter.$.close()
      conversationAPI?.conversationHandler.$.close()
    }
  }, [conversationAPI])

  const handleNewConversation = () => {
    setConversationAPI(API.newConversation())
    setSelectedConversationUuid(undefined)
  }

  React.useEffect(() => {
    // Observe when default conversation changes
    const onNewDefaultConversationListenerId =
      getAPI().UIObserver.onNewDefaultConversation.addListener(() => {
        setDefaultConversationToken(new Date().getTime())
      })

    return () => {
      getAPI().UIObserver.removeListener(onNewDefaultConversationListenerId)
    }
  }, [])

  return (
    <AIChatContextProvider
      selectedConversationUuid={selectedConversationUuid}
      onNewConversation={handleNewConversation}
      onSelectConversationUuid={handleSelectConversationUuid}
    >
      {conversationAPI && (
        <ConversationContextProvider {...conversationAPI}>
          <BraveCoreThemeProvider>
            <Main />
          </BraveCoreThemeProvider>
        </ConversationContextProvider>
      )}
    </AIChatContextProvider>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
