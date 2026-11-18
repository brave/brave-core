// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useAIChat } from '../state/ai_chat_context'
import { useConversation } from '../state/conversation_context'

/**
 * Whether we should offer the user the ability to start a new conversation.
 * Attempts to debounce when moving from one conversation to another, and ignore
 * the in-between state of no conversation entries during fetch of a new
 * conversation.
 */
export default function useCanStartNewConversation() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const {
    isPlaceholderData: isConversationLoading,
    getConversationHistoryData: conversationHistory,
  } = conversationContext.api.useGetConversationHistory()

  const canStartNewConversation =
    aiChatContext.hasAcceptedAgreement
    && (isConversationLoading || !!conversationHistory.length)

  return canStartNewConversation
}
