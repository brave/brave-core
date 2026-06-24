// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './load_time_data'
import '@brave/leo/tokens/css/variables.css'
import '$web-common/defaultTrustedTypesPolicy'
import '../../../../ui/webui/resources/css/reset.css'

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import * as Mojom from '../common/mojom'
import { parseConversationData } from '../common/conversation_serialization'
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

// Set the nala icon path to be relative to this script, which should be
// the root of the output bundle.
// Need to store import.meta.url in a separate variable to the URL building
// otherwise webpack will try to resolve '.' locally (and probably fail).
const scriptUrl = import.meta.url
const relativePathUrl = new URL('./nala-icons', scriptUrl)
setIconBasePath(relativePathUrl.toString())

/**
 * Create a minimal local-only read-only version of the AI Chat API interfaces.
 *
 * Builds an UntrustedConversationAPI instance backed entirely by mock Mojo
 * interfaces. Reads return hard-coded/empty data and all writes are no-ops, so
 * the conversation renders purely from data supplied locally via
 * displayConversation().
 *
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

const api = createLocalConversationApi()

/**
 * Renders the supplied conversation in place.
 *
 * @param conversationDataRaw - conversation data as stringified by common/conversation_serialization.ts
 * @param element - the DOM element to render the conversation into
 */
export function renderConversation(
  conversationDataRaw: string,
  element: HTMLElement,
) {
  let conversation: Mojom.ConversationTurn[]

  try {
    conversation = parseConversationData(conversationDataRaw)
  } catch (e) {
    console.error('Failed to parse conversation data', e)
    element.textContent = 'Failed to load conversation'
    return
  }

  console.log('conversation', conversation)

  api.getConversationHistory.update(conversation)

  const root = createRoot(element)

  root.render(
    <div style={{ backgroundColor: 'var(--leo-color-container-background)' }}>
      <UntrustedConversationContextProvider api={api}>
        <Conversation />
      </UntrustedConversationContextProvider>
    </div>,
  )
}
