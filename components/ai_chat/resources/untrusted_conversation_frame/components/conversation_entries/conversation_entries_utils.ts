// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import remarkGfm from 'remark-gfm'
import remarkParse from 'remark-parse'
import { unified } from 'unified'
import { visit } from 'unist-util-visit'

import * as Mojom from '../../../common/mojom'

const NON_TASK_TOOL_NAMES = [
  Mojom.USER_CHOICE_TOOL_NAME,
  Mojom.MEMORY_STORAGE_TOOL_NAME,
]

// How many task tools need to occur within a group for the group
// to be considered a task.
const TASK_TOOL_COUNT = 2

/**
 * Groups consecutive assistant entries for the purposes of combining tool use
 * loops. Each tool use and response results in a separate ConversationTurn, but
 * we want to combine them in the UI as the same turn. Other entries will form a
 * group each with only a single entry.
 * @param allEntries All ungrouped conversation entries for a conversation
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

/**
 * A task is when there are multiple entries within a group and there
 * are at least 2 task tool use events and 1 completion event.
 * @param group Group of assistant conversation entries from groupConversationEntries
 * @returns true if the group is a task
 */
export function isAssistantGroupTask(group: Mojom.ConversationTurn[]) {
  // Must have at least 1 tool use and a response to the tool use (2 group entries)
  if (group.length <= 1) {
    return false
  }

  // Must have at least 2 task tool uses within the group of responses
  let taskToolCount = 0
  // Must have at least 1 completion event
  let hasCompletion = false
  for (const entry of group) {
    // Sanity check: shouldn't have any non-assistant entries
    if (entry.characterType !== Mojom.CharacterType.ASSISTANT) {
      console.error(
        'isAssistantGroupTask: non-assistant entry found in group',
        entry,
      )
      return false
    }
    if (entry.events) {
      for (const event of entry.events) {
        if (event.completionEvent) {
          hasCompletion = true
        }
        if (
          !!event.toolUseEvent
          && !NON_TASK_TOOL_NAMES.includes(event.toolUseEvent.toolName)
        ) {
          taskToolCount++
        }
        // Optimization: stop iterating events if we have what we need
        if (hasCompletion && taskToolCount >= TASK_TOOL_COUNT) {
          return true
        }
      }
    }
  }

  return false
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

// Applies `transform` to every part of `text` that lies outside a markdown
// fenced code block or inline code span. Code regions pass through unchanged,
// so bracket-number syntax inside `arr[1]` etc. is preserved.
const applyOutsideCodeBlocks = (
  text: string,
  transform: (segment: string) => string,
): string => {
  const tree = unified().use(remarkParse).use(remarkGfm).parse(text)
  const ranges: Array<[number, number]> = []
  visit(tree, (node) => {
    if (node.type !== 'code' && node.type !== 'inlineCode') return
    const start = node.position?.start.offset
    const end = node.position?.end.offset
    if (start !== undefined && end !== undefined) {
      ranges.push([start, end])
    }
  })
  if (ranges.length === 0) return transform(text)

  let result = ''
  let cursor = 0
  for (const [start, end] of ranges) {
    result += transform(text.slice(cursor, start))
    result += text.slice(start, end)
    cursor = end
  }
  result += transform(text.slice(cursor))
  return result
}

export const removeCitationsWithMissingLinks = (
  text: string,
  citationLinks: string[],
) =>
  applyOutsideCodeBlocks(text, (segment) =>
    segment.replace(/\[(\d+)\]/g, (match, citationNumber) => {
      // Convert to 0-based index
      const index = parseInt(citationNumber) - 1
      return index >= 0 && index < citationLinks.length ? match : ''
    }),
  )

/**
 * Normalizes citation spacing so markdown parses each citation as its own link.
 * - Separates consecutive citations: [2][3] → [2] [3] (so [2][3] isn't parsed
 *   as one link with text "2" and ref [3]).
 * - Adds space before a citation when it runs onto the previous word:
 *   "Japan[2]" → "Japan [2]".
 * Code blocks and inline code are left untouched so that array indexing like
 * `arr[1]` is not mangled into `arr [1]`.
 */
export const normalizeCitationSpacing = (text: string): string =>
  applyOutsideCodeBlocks(text, (segment) => {
    const withSeparatedCitations = segment.replace(/\]\s*\[/g, '] [')
    return withSeparatedCitations.replace(/(\w|\S)\[(\d+)\]/g, '$1 [$2]')
  })

/**
 * Replaces citation numbers `[1]`, `[2]`, etc. with URLs from `allowedLinks`,
 * skipping matches that fall inside fenced code blocks or inline code spans —
 * where `[N]` is typically array indexing rather than a citation.
 */
export const replaceCitationsWithUrlsExcludingCode = (
  text: string,
  allowedLinks: string[],
): string => {
  if (allowedLinks.length === 0) {
    return text
  }
  return applyOutsideCodeBlocks(text, (segment) =>
    segment.replace(/\[(\d+)\]/g, (match, citationNumber, offset) => {
      const index = parseInt(citationNumber) - 1
      if (index >= 0 && index < allowedLinks.length) {
        const url = allowedLinks[index]
        const charBefore = offset > 0 ? segment[offset - 1] : ''
        const needsSpace = charBefore && !/\s/.test(charBefore)
        return needsSpace ? ` ${url}` : url
      }
      return match
    }),
  )
}

/**
 * Collects all tool artifacts from the events of a group of conversation turns.
 * Deduplicates artifacts by id, keeping only the latest artifact for each id.
 * Artifacts without an id are always included.
 */
export function getToolArtifacts(
  group: Mojom.ConversationTurn[],
): Mojom.ToolArtifact[] {
  const artifactsWithoutId: Mojom.ToolArtifact[] = []
  const artifactsById = new Map<string, Mojom.ToolArtifact>()

  for (const entry of group) {
    if (!entry.events) {
      continue
    }

    for (const event of entry.events) {
      if (!event.toolUseEvent?.artifacts) {
        continue
      }
      for (const artifact of event.toolUseEvent.artifacts) {
        if (artifact.id) {
          artifactsById.set(artifact.id, artifact)
        } else {
          artifactsWithoutId.push(artifact)
        }
      }
    }
  }

  return [...artifactsWithoutId, ...artifactsById.values()]
}
