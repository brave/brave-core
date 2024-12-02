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
import API from './api_conversation_entries_ui'
import ConversationEntries from './components/conversation_entries'
import { ConversationEntriesContextProvider } from './conversation_entries_context'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App() {
  return (
    <ConversationEntriesContextProvider>
      <ConversationEntries
        onLastElementHeightChange={
          () => API.getInstance().parentUIFrame.generatedConversationEntryHeightChanged()
        }
      />
    </ConversationEntriesContextProvider>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
