// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  extractAllowedLinksFromTurn,
  replaceCitationsWithUrls,
} from '../../../common/conversation_history_utils'

// Possibly expose an event handler for copying the entry's text to clipboard,
// if the entry is simple enough for the text to be extracted
// (i.e. no tool use events).
export default function useConversationEventClipboardCopyHandler(
  entry: Mojom.ConversationTurn,
) {
  // Cannot copy complicated structured content (for now)
  if (entry.events?.some((event) => !!event.toolUseEvent)) {
    return undefined
  }

  return () => {
    if (entry.characterType === Mojom.CharacterType.ASSISTANT) {
      const event = entry.events?.find((event) => event.completionEvent)
      if (!event?.completionEvent) return
      let text = event.completionEvent.completion
      // Replace citations with URLs
      const allowedLinks = extractAllowedLinksFromTurn(entry.events)
      text = replaceCitationsWithUrls(text, allowedLinks)
      navigator.clipboard.writeText(text)
    } else {
      navigator.clipboard.writeText(entry.text)
    }
  }
}
