// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  extractAllowedLinksFromTurn,
  inlineSearchRegex,
} from '../../../common/conversation_history_utils'
import { replaceCitationsWithUrlsExcludingCode } from './conversation_entries_utils'

// Possibly expose an event handler for copying the entry's text to clipboard,
// if the entry is simple enough for the text to be extracted
export default function useConversationEventClipboardCopyHandler(
  group: Mojom.ConversationTurn[],
) {
  return () => {
    const entry = group[0]
    if (entry.characterType === Mojom.CharacterType.ASSISTANT) {
      // Collect completion text from all turns in the group (e.g. tool use
      // turns split the completion across multiple entries) and from all
      // completion events within each turn (an inline-search event between
      // streaming chunks splits the completion stream into multiple events).
      const allEvents = group.flatMap((turn) => turn.events ?? [])
      const completionTexts = group.map((turn) =>
        (turn.events ?? [])
          .filter((e) => e.completionEvent)
          .map((e) => e.completionEvent!.completion)
          .join(''),
      )
      let text = completionTexts.filter(Boolean).join('\n\n') || entry.text
      if (!text) return
      // Replace citations with URLs, skipping matches inside code blocks where
      // `[N]` is typically array indexing rather than a citation.
      const allowedLinks = extractAllowedLinksFromTurn(allEvents)
      text = replaceCitationsWithUrlsExcludingCode(text, allowedLinks)
      text = text.replaceAll(inlineSearchRegex, '')
      navigator.clipboard.writeText(text)
    } else {
      navigator.clipboard.writeText(entry.text)
    }
  }
}
