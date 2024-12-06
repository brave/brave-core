// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '@brave/leo/tokens/css/variables.css'
import '$web-components/app.global.scss'
import '$web-common/defaultTrustedTypesPolicy'
import ConversationEntries from './components/conversation_entries'
import { UntrustedConversationContextProvider } from './untrusted_conversation_context'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App() {
  return (
    <UntrustedConversationContextProvider>
      <ConversationEntries />
    </UntrustedConversationContextProvider>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
