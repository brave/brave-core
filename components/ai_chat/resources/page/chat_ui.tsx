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
import { AIChatContextProvider, useAIChat } from './state/ai_chat_context'
import Main from './components/main'
import {
  ConversationContextProvider
} from './state/conversation_context'
import FullScreen from './components/full_page'
import { ActiveChatProviderFromUrl } from './state/active_chat_context'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App() {
  React.useEffect(() => {
    document.getElementById('mountPoint')?.classList.add('loaded')
  }, [])

  return (
    <AIChatContextProvider>
      <ActiveChatProviderFromUrl>
        <ConversationContextProvider>
          <BraveCoreThemeProvider>
            <Content />
          </BraveCoreThemeProvider>
        </ConversationContextProvider>
      </ActiveChatProviderFromUrl>
    </AIChatContextProvider>
  )
}

function Content() {
  const aiChatContext = useAIChat()

  if (aiChatContext.isStandalone === undefined) {
    return <div>loading...</div>
  }

  if (!aiChatContext.isStandalone) {
    return <Main />
  }

  return <FullScreen />
}

function initialize() {
  initLocale(loadTimeData.data_)
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
