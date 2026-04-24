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
  group: Mojom.ConversationTurn[],
) {
  return () => {
    const entry = group[0]
    if (entry.characterType === Mojom.CharacterType.ASSISTANT) {
      // Collect completion text from all turns in the group (e.g. tool use turns
      // split the completion across multiple entries in the group).
      const allEvents = group.flatMap((turn) => turn.events ?? [])
      const completionTexts = group.flatMap((turn) => {
        const event = turn.events?.findLast((e) => e.completionEvent)
        return event?.completionEvent?.completion
          ? [event.completionEvent.completion]
          : []
      })
      let text = completionTexts.join('\n\n') || entry.text
      if (!text) return
      // Replace citations with URLs
      const allowedLinks = extractAllowedLinksFromTurn(allEvents)
      text = replaceCitationsWithUrls(text, allowedLinks)
      text = text.replaceAll(inlineSearchRegex, '')
      navigator.clipboard.writeText(text)
    } else {
      navigator.clipboard.writeText(entry.text)
    }
  }
}
