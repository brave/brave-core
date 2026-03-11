// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  extractAllowedLinksFromTurn,
  replaceCitationsWithUrls,
} from '../../../common/conversation_history_utils'

// Note: This regex is used to strip out inline searches, as they're not useful
// for the user.
const inlineSearchRegex = /^::search\[.+?\]{type=\w+?}$/gm

// Possibly expose an event handler for copying the entry's text to clipboard,
// if the entry is simple enough for the text to be extracted
export default function useConversationEventClipboardCopyHandler(
  entry: Mojom.ConversationTurn,
) {
  return () => {
    if (entry.characterType === Mojom.CharacterType.ASSISTANT) {
      const event = entry.events?.findLast((event) => event.completionEvent)
      if (!event?.completionEvent) return
      let text = event.completionEvent.completion
      // Replace citations with URLs
      const allowedLinks = extractAllowedLinksFromTurn(entry.events)
      text = replaceCitationsWithUrls(text, allowedLinks)
      text = text.replaceAll(inlineSearchRegex, '')
      navigator.clipboard.writeText(text)
    } else {
      navigator.clipboard.writeText(entry.text)
    }
  }
}
