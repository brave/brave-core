// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'

/**
 * Groups consecutive assistant entries. Other entries will form a group each
 * with only a single entry.
 * @param allEntries Flat list of conversation entries
 */
export function groupConversationEntries(
  allEntries: Mojom.ConversationTurn[],
): Mojom.ConversationTurn[][] {
  const groupedEntries: Mojom.ConversationTurn[][] = []
  for (const entry of allEntries) {
    const latestGroup = groupedEntries[groupedEntries.length - 1]
    if (
      !latestGroup?.length
      || entry.characterType !== Mojom.CharacterType.ASSISTANT
      || latestGroup[0]?.characterType !== Mojom.CharacterType.ASSISTANT
    ) {
      groupedEntries.push([entry])
      continue
    }
    if (
      entry.characterType === Mojom.CharacterType.ASSISTANT
      && latestGroup[0]?.characterType === Mojom.CharacterType.ASSISTANT
    ) {
      latestGroup.push(entry)
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

export const removeReasoning = (text: string) => {
  const openingTagIndex = text.indexOf('<think>')
  const closingTagIndex = text.indexOf('</think>')

  // If there is an opening tag but no closing tag,
  // return the text before the opening tag.
  if (openingTagIndex !== -1 && closingTagIndex === -1) {
    return text.substring(0, openingTagIndex)
  }

  // If there is a closing tag but no opening tag,
  // return the text after the closing tag.
  if (closingTagIndex !== -1 && openingTagIndex === -1) {
    return text.substring(closingTagIndex + '</think>'.length)
  }

  // If there is an opening tag and a closing tag,
  // return the text before the opening tag and the text after the closing tag.
  if (openingTagIndex !== -1 && closingTagIndex !== -1) {
    const textBeforeOpeningTag = text.substring(0, openingTagIndex)
    const textAfterClosingTag = text.substring(
      closingTagIndex + '</think>'.length,
    )
    return textBeforeOpeningTag + textAfterClosingTag
  }

  // If there is no opening or closing tag, return the original text.
  return text
}

export const removeCitationsWithMissingLinks = (
  text: string,
  citationLinks: string[],
) => {
  return text.replace(/\[(\d+)\]/g, (match, citationNumber) => {
    // Convert to 0-based index
    const index = parseInt(citationNumber) - 1
    return index >= 0 && index < citationLinks.length ? match : ''
  })
}
