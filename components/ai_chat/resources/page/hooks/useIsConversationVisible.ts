// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useMemo } from "react";
import { useAIChat } from "../state/ai_chat_context";

export function useGetVisibleConversation(conversationUuid?: string) {
  const context = useAIChat()

  return useMemo(() => {
    if (!conversationUuid) {
      return undefined
    }
    return context.visibleConversations.find(c => c.uuid === conversationUuid)
  }, [conversationUuid, context.visibleConversations])
}

export default function useIsConversationVisible(conversationUuid?: string) {
  return (useGetVisibleConversation(conversationUuid) !== undefined)
}
