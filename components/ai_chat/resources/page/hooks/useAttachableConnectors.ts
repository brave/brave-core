// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../common/mojom'
import { findConnectorContent } from '../../common/conversation_history_utils'
import { useAIChat } from '../state/ai_chat_context'
import { useConversation } from '../state/conversation_context'
import { getAttachableConnectors } from '../components/connectors_modal/connector_utils'

/**
 * Whether the user's connectors can be attached to the conversation: they're
 * offered here, the model can use their tools, and there's a conversation to
 * attach them to.
 */
export function useCanAttachConnectors(): boolean {
  const aiChat = useAIChat()
  const conversation = useConversation()
  const { conversationUuid } = conversation.api.useGetStateData()
  return (
    aiChat.isMCPEnabled
    && !!conversation.currentModel?.supportsTools
    && !!conversationUuid
  )
}

export interface AttachableConnectors {
  servers: Mojom.MCPServer[]
  /**
   * Attaches the connector to the conversation, or turns its tools back on if
   * it's attached with them off.
   */
  attach: (server: Mojom.MCPServer) => void
  /** Detaches the connector, leaving focus where it is. */
  detach: (content: Mojom.AssociatedContent) => void
}

/**
 * The user's connectors that can be attached to the conversation. It asks the
 * browser for them, so only call it where `useCanAttachConnectors()` returns
 * true.
 */
export default function useAttachableConnectors(): AttachableConnectors {
  const aiChat = useAIChat()
  const conversation = useConversation()
  const { conversationUuid, associatedContent } =
    conversation.api.useGetStateData()
  const { getServersData } = aiChat.api.useGetServers()

  const servers = React.useMemo(
    () => getAttachableConnectors(getServersData ?? []),
    [getServersData],
  )

  const attach = (server: Mojom.MCPServer) => {
    const content = findConnectorContent(associatedContent, server.id)
    if (content) {
      conversation.setToolsAttached(content, true)
    } else if (conversationUuid) {
      aiChat.api.uiHandler.associateMCPServer(server.id, conversationUuid)
    }
  }

  const detach = (content: Mojom.AssociatedContent) =>
    conversation.disassociateContent(content, { keepFocus: true })

  return { servers, attach, detach }
}
