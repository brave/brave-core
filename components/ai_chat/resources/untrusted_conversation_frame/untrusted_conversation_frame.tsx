// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-common/defaultTrustedTypesPolicy'
import ConversationEntries from './components/conversation_entries'
import { UntrustedConversationContextProvider } from './untrusted_conversation_context'
import { untrustedFrameDragHandlingSetup } from './hooks/useUntrustedFrameDragHandling'
// <if expr="is_ios">
import { useIOSOneTapFix } from '../common/useIOSOneTapFix'
import UntrustedConversationFrameAPI from './untrusted_conversation_frame_api'
// </if>

import '../common/strings'

// <if expr="is_ios">
function onTapElsewhereDismissMenus() {
  UntrustedConversationFrameAPI.getInstance().parentUIFrame.dismissMenus()
}
// </if>
import {
  bindUntrustedConversation,
  BoundUntrustedConversation,
} from './api/bind_untrusted_conversation'

setIconBasePath('chrome-untrusted://resources/brave-icons')

// Set up drag handling at module level
untrustedFrameDragHandlingSetup()

interface AppProps {
  boundConversation: BoundUntrustedConversation
}

function App(props: AppProps) {
  // <if expr="is_ios">
  // One-tap fix for iframe menus; notify parent when user taps on
  // non-interactive content so parent can close menus.
  useIOSOneTapFix({ onTapElsewhere: onTapElsewhereDismissMenus })
  // </if>
  return (
    <UntrustedConversationContextProvider api={props.boundConversation.api}>
      <ConversationEntries />
    </UntrustedConversationContextProvider>
  )
}

async function initialize() {
  // Bind the conversation and initialize the API
  const boundConversation = await bindUntrustedConversation()

  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App boundConversation={boundConversation} />)
}

document.addEventListener('DOMContentLoaded', initialize)
