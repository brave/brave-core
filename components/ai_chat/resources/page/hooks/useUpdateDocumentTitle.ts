// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../state/ai_chat_context'
import { useConversation } from '../state/conversation_context'

/**
 * Keep the document title updated with the current conversation according to
 * the nearest ConversationContext
 */
export default function useUpdateDocumentTitle() {
  const aiChat = useAIChat()
  const conversations = aiChat.api.useGetConversations().data
  const { conversationUuid } = useConversation().api.useGetStateData()

  // Update page title when conversation changes
  React.useEffect(() => {
    const originalTitle = document.title
    const conversationTitle =
      conversations.find((c) => c.uuid === conversationUuid)?.title
      || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)

    function setTitle(isPWA: boolean) {
      if (isPWA) {
        document.title = conversationTitle
      } else {
        document.title = `${getLocale(S.CHAT_UI_TITLE)} - ${conversationTitle}`
      }
    }

    const isPWAQuery = window.matchMedia('(display-mode: standalone)')
    const handleChange = (e: MediaQueryListEvent) => setTitle(e.matches)
    isPWAQuery.addEventListener('change', handleChange)

    setTitle(isPWAQuery.matches)

    return () => {
      document.title = originalTitle
      isPWAQuery.removeEventListener('change', handleChange)
    }
  }, [conversations, conversationUuid])
}
