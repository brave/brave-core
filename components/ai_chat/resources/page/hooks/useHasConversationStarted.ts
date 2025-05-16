// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useMemo } from "react";
import { useAIChat } from "../state/ai_chat_context";
import * as Mojom from "../../common/mojom";

export default function useHasConversationStarted(conversationId?: string) {
    const context = useAIChat()

    return useMemo<boolean>(
      () => !!context.conversations.find(
        (c: Mojom.Conversation) => c.uuid === conversationId && c.hasContent
      ),
      [conversationId, context.conversations]
    )
}
