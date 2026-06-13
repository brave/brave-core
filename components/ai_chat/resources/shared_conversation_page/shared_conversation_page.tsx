// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './load_time_data'
import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import '$web-common/defaultTrustedTypesPolicy'
import * as Mojom from '../common/mojom'
import Conversation from '../untrusted_conversation_frame/components/conversation'
import { UntrustedConversationContextProvider } from '../untrusted_conversation_frame/untrusted_conversation_context'
import createUntrustedConversationApi, {
  UntrustedConversationAPI,
} from '../untrusted_conversation_frame/api/untrusted_conversation_api'
import {
  createMockParentUIFrame,
  createMockUntrustedConversationHandler,
  createMockUntrustedService,
  createMockUntrustedUIHandler,
} from '../untrusted_conversation_frame/api/mock_interfaces'
import { deserializeConversation } from '../common/conversation_serialization'
import DemoSharedConversationInput from './demo_shared_conversation_input'
import '@brave/leo/tokens/css/variables.css'
import '../../../../ui/webui/resources/css/reset.css'
import '../../../../ui/webui/resources/fonts/manrope.css'
import '../../../../ui/webui/resources/fonts/poppins.css'
import '../../../../ui/webui/resources/fonts/inter.css'
import '../page/styles.css'

setIconBasePath('/nala-icons')

/**
 * Builds an UntrustedConversationAPI instance backed entirely by mock Mojo
 * interfaces. Reads return hard-coded/empty data and all writes are no-ops, so
 * the conversation renders purely from data supplied locally via
 * displayConversation().
 */
function createLocalConversationApi(): UntrustedConversationAPI {
  const conversationHandler = createMockUntrustedConversationHandler()
  const uiHandler = createMockUntrustedUIHandler()
  const parentUIFrame = createMockParentUIFrame()
  const service = createMockUntrustedService()

  return createUntrustedConversationApi(
    conversationHandler,
    uiHandler,
    parentUIFrame,
    service,
  ).api
}

// Created eagerly at module load so that displayConversation() can be called
// before (or after) the DOM is ready - the data is stored on the API regardless
// of whether the React tree has mounted yet.
const api = createLocalConversationApi()

/**
 * Renders the supplied conversation in place. The conversation is a plain array
 * of mojom ConversationTurn objects (e.g. parsed from local JSON). Calling this
 * again replaces the currently displayed conversation.
 */
export function displayConversation(conversation: Mojom.ConversationTurn[]) {
  api.getConversationHistory.update(conversation)
}

/**
 * Renders a conversation from a JSON string produced by the AI Chat page's
 * "Export conversation" action (see common/conversation_serialization.ts).
 */
export function displayConversationFromJson(json: string) {
  displayConversation(deserializeConversation(json))
}

// Expose for callers that load this bundle and drive it from the page host
// (e.g. via inline script or another bundle).
Object.assign(window, { displayConversation, displayConversationFromJson })

function App() {
  return (
    <UntrustedConversationContextProvider api={api}>
      <DemoSharedConversationInput onLoad={displayConversation} />
      <Conversation />
    </UntrustedConversationContextProvider>
  )
}

function initialize() {
  const root = createRoot(document.getElementById('mountPoint')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
