// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Conversation from '../../../untrusted_conversation_frame/components/conversation'
import { ConversationEntriesProps } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'

// The real WebUI has an iframe with the ScrollableContent component but
// in storybook it's easier to just include it directly.
export default function StorybookConversationEntries(
  props: ConversationEntriesProps,
) {
  const conversationContext = useConversation()

  return <Conversation key={conversationContext.conversationUuid} />
}
