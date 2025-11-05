/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { useAIChat } from '../../../../../components/ai_chat/resources/page/state/ai_chat_context'

export function useConversationList() {
  const aiChatContext = useAIChat()
  return aiChatContext.conversations
    .filter((c) => !c.temporary && c.hasContent)
    .map((c) => ({ uuid: c.uuid, title: c.title }))
}
