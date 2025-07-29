/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as API from '../../../../components/ai_chat/resources/page/api'
import { AIChatContextProvider } from '../../../../components/ai_chat/resources/page/state/ai_chat_context'
import {
  ActiveChatContext,
  SelectedChatDetails,
} from '../../../../components/ai_chat/resources/page/state/active_chat_context'
import { ConversationContextProvider } from '../../../../components/ai_chat/resources/page/state/conversation_context'
import '../../../../components/ai_chat/resources/common/strings'

export function AIChatProvider(props: { children: React.ReactNode }) {
  return (
    <AIChatContextProvider conversationEntriesComponent={() => <></>}>
      <NewTabActiveChatProvider>
        <ConversationContextProvider>
          {props.children}
        </ConversationContextProvider>
      </NewTabActiveChatProvider>
    </AIChatContextProvider>
  )
}

function NewTabActiveChatProvider(props: { children: React.ReactNode }) {
  const [details, setDetails] = React.useState<SelectedChatDetails>()

  React.useEffect(() => {
    setDetails({
      ...API.newConversation(),
      selectedConversationId: undefined,
      updateSelectedConversationId: () => {},
      createNewConversation: () => {},
      isTabAssociated: false,
    })
  }, [])

  if (!details) {
    return null
  }

  return (
    <ActiveChatContext.Provider value={details}>
      {props.children}
    </ActiveChatContext.Provider>
  )
}
