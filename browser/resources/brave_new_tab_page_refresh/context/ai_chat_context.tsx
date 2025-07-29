/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AIChatContextProvider } from '../../../../components/ai_chat/resources/page/state/ai_chat_context'
import { ActiveChatProvider } from '../../../../components/ai_chat/resources/page/state/active_chat_context'
import { ConversationContextProvider } from '../../../../components/ai_chat/resources/page/state/conversation_context'
import '../../../../components/ai_chat/resources/common/strings'

import { useNewTabState } from './new_tab_context'

export function AIChatProvider(props: { children: React.ReactNode }) {
  const aiChatFeatureEnabled = useNewTabState((s) => s.aiChatFeatureEnabled)
  if (!aiChatFeatureEnabled) {
    return <>{props.children}</>
  }
  return (
    <AIChatContextProvider conversationEntriesComponent={() => <></>}>
      <ActiveChatProvider
        selectedConversationId=''
        updateSelectedConversationId={() => {}}
      >
        <ConversationContextProvider>
          {props.children}
        </ConversationContextProvider>
      </ActiveChatProvider>
    </AIChatContextProvider>
  )
}
