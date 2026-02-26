/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { newConversation } from '../../../../components/ai_chat/resources/page/api/bind_conversation'
import bindAiChatWebUiServices from '../../../../components/ai_chat/resources/page/api/bind_webui_services'
import {
  ActiveChatContext,
  SelectedChatDetails,
} from '../../../../components/ai_chat/resources/page/state/active_chat_context'
import { AIChatProvider } from '../../../../components/ai_chat/resources/page/state/ai_chat_context'
import { ConversationProvider } from '../../../../components/ai_chat/resources/page/state/conversation_context'
import '../../../../components/ai_chat/resources/common/strings'

export default function AIChatContextsProvider(props: { children: React.ReactNode }) {
  const aiChatBindings = React.useMemo(bindAiChatWebUiServices, [])

  const conversationDetails = React.useMemo<SelectedChatDetails>(() => {
    const conversationBindings = newConversation(aiChatBindings.api)
    return {
      ...conversationBindings,
      selectedConversationId: undefined,
      updateSelectedConversationId: () => {},
      createNewConversation: () => {},
      isTabAssociated: false,
    }
  }, [])

  return (
    <AIChatProvider
      api={aiChatBindings.api}
      conversationEntriesComponent={() => (<div />)}
    >
      <ActiveChatContext.Provider value={conversationDetails}>
        <ConversationProvider {...conversationDetails}>
          {props.children}
        </ConversationProvider>
      </ActiveChatContext.Provider>
    </AIChatProvider>
  )
}
