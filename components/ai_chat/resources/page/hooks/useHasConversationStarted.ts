// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useMemo } from 'react'
import { useAIChat } from '../state/ai_chat_context'

export default function useHasConversationStarted(conversationId?: string) {
  const { getConversationsData } = useAIChat().api.useGetConversations()

  return useMemo<boolean>(() => {
    if (!conversationId || !getConversationsData) {
      return false
    }
    return getConversationsData.some((c) => c.uuid === conversationId)
  }, [conversationId, getConversationsData])
}
