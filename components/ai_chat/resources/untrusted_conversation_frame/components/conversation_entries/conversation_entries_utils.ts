// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'

/**
 * Combines events of entries if there are multiple consecutive assistant entries.
 * @param allEntries Flat list of conversation entries
 */
export function groupConversationEntries(allEntries: Mojom.ConversationTurn[]): Mojom.ConversationTurn[][] {
  // TODO: test
  const groupedEntries: Mojom.ConversationTurn[][] = []
  for (const entry of allEntries) {
    const lastEntry = groupedEntries[groupedEntries.length - 1]
    if (!groupedEntries.length || !lastEntry?.length || entry.characterType !== Mojom.CharacterType.ASSISTANT || lastEntry[0]?.characterType !== Mojom.CharacterType.ASSISTANT) {
      groupedEntries.push([entry])
      continue
    }
    if (entry.characterType === Mojom.CharacterType.ASSISTANT && lastEntry[0]?.characterType === Mojom.CharacterType.ASSISTANT) {
      groupedEntries[groupedEntries.length - 1].push(entry)
      continue
    }
  }
  return groupedEntries
}

export function getReasoningText(text: string) {
  const startTag = '<think>'
  const endTag = '</think>'
  let result = ''
  let startIndex = -1
  let tagDepth = 0
  let i = 0

  while (i < text.length) {
    // Check for startTag
    if (text.substring(i, i + startTag.length) === startTag) {
      if (tagDepth === 0) {
        startIndex = i + startTag.length // Mark beginning of content
      }
      tagDepth++
      i += startTag.length
    }
    // Check for endTag
    else if (text.substring(i, i + endTag.length) === endTag) {
      if (tagDepth > 0) {
        tagDepth--
        if (tagDepth === 0 && startIndex !== -1) {
          // Complete section found
          result = text.substring(startIndex, i)
          break
        }
      }
      i += endTag.length
    } else {
      i++
    }
  }

  // If we have an open section (streaming case) and no complete closure
  if (tagDepth > 0 && startIndex !== -1) {
    result = text.substring(startIndex)
  }

  // Clean up any remaining tags
  while (result.includes(startTag) || result.includes(endTag)) {
    result = result.replace(startTag, '').replace(endTag, '')
  }

  return result.trim()
}
