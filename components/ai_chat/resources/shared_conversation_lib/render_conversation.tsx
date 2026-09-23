// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './load_time_data'
import './set_icon_base_path'
import '$web-common/defaultTrustedTypesPolicy'
import '../../../../ui/webui/resources/css/reset.css'

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import StyledComponentsProvider from '$web-common/StyledComponentsProvider'
import {
  parseConversationData,
  type ConversationData,
} from '../common/conversation_serialization'
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
import setupRenderingElement from './setup_rendering_element'

/**
 * Since the shared conversation viewer can read from different versions
 * of this code, we should try to retain backwards-compatibility with these
 * fields.
 */
type RenderConversationResult = {
  conversationTitle?: string
  isError: boolean
}

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
): RenderConversationResult {
  let conversation: ConversationData

  try {
    conversation = parseConversationData(conversationDataRaw)
  } catch (e) {
    console.error('Failed to parse conversation data', e)
    element.textContent = 'Failed to load conversation'
    return {
      isError: true,
    }
  }

  api.getConversationHistory.update(conversation.messages)
  // Conversations shared before associated content was included in the payload
  // don't have the property.
  api.associatedContent.update(conversation.associatedContent ?? [])

  // Render to a shadow DOM to avoid style conflicts with the hosting page
  const container = setupRenderingElement(element)
  const root = createRoot(container)

  root.render(
    <StyledComponentsProvider>
      <UntrustedConversationContextProvider
        api={api}
        isReadOnly
      >
        <Conversation />
      </UntrustedConversationContextProvider>
    </StyledComponentsProvider>,
  )

  return {
    conversationTitle: conversation.title,
    isError: false,
  }
}
