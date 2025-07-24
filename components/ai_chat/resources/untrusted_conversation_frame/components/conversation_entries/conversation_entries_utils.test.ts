// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  getReasoningText,
  removeReasoning,
  removeCitationsWithMissingLinks,
  groupConversationEntries,
} from './conversation_entries_utils'
import * as Mojom from '../../../common/mojom'

describe('groupConversationEntries', () => {
  it('should group consecutive assistant entries', () => {
    const turn1 = {
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        { completionEvent: { completion: 'Response 1' } },
        {
          sourcesEvent: {
            sources: [{ url: { url: 'https://a.com' }, title: 'Title 1', faviconUrl: { url: 'https://a.com/favicon.ico' } }]
        }
      }
    ]
  }

    const turn2 = {
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        { completionEvent: { completion: 'Response 2' } },
        {
          sourcesEvent: {
            sources: [{ url: { url: 'https://b.com' }, title: 'Title 2', faviconUrl: { url: 'https://b.com/favicon.ico' } }]
        }
      }
    ]
  }

    const groupedEntries = groupConversationEntries([turn1, turn2] as any)

    expect(groupedEntries).toHaveLength(1)
    expect(groupedEntries[0]).toHaveLength(2)
    expect(groupedEntries[0]).toEqual([turn1, turn2])
  })

  it('should not group non-consecutive assistant entries', () => {
    const assistantTurn1 = {
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        { completionEvent: { completion: 'Response 1' } },
        {
          sourcesEvent: {
            sources: [{ url: { url: 'https://a.com' }, title: 'Title 1', faviconUrl: { url: 'https://a.com/favicon.ico' } }]
          }
        }
      ]
    }

    const humanTurn1: Partial<Mojom.ConversationTurn> = {
      characterType: Mojom.CharacterType.HUMAN,
      text: 'Question 1',
    }

    const assistantTurn2 = {
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        { completionEvent: { completion: 'Response 2' } },
        {
          sourcesEvent: {
            sources: [{ url: { url: 'https://b.com' }, title: 'Title 2', faviconUrl: { url: 'https://b.com/favicon.ico' } }]
          }
        }
      ]
    }

    const groupedEntries = groupConversationEntries([assistantTurn1, humanTurn1, assistantTurn2] as any)

    expect(groupedEntries).toHaveLength(3)
    expect(groupedEntries[0]).toEqual([assistantTurn1])
    expect(groupedEntries[1]).toEqual([humanTurn1])
    expect(groupedEntries[2]).toEqual([assistantTurn2])
  })
})

describe('getReasoningText', () => {
  it('Should extract reasoning text between start and end tags', () => {
    const input = '<think>Reasoning text here.</think>'
    expect(getReasoningText(input)).toBe('Reasoning text here.')
  })

  it('Should handle missing end tag by returning the rest of the text', () => {
    const input = '<think>Start of reasoning without end.'
    expect(getReasoningText(input)).toBe('Start of reasoning without end.')
  })

  it('Should handle missing start tag by returning an empty string', () => {
    const input = 'Reasoning text with no start tag.</think>'
    expect(getReasoningText(input)).toBe('')
  })

  it('Should handle nested think tags and remove them from the string', () => {
    const input =
      '<think>Reasoning text <think>with nested</think> tags.</think>'
    expect(getReasoningText(input)).toBe('Reasoning text with nested tags.')
  })

  it('Should handle removing white space around reasoning text', () => {
    const input = '<think> Reasoning text here. </think>'
    expect(getReasoningText(input)).toBe('Reasoning text here.')
  })
})

describe('removeReasoning', () => {
  it('Should remove reasoning text between start and end tags', () => {
    const input = '<think>Reasoning text here.</think> Rest of the text.'
    expect(removeReasoning(input)).toBe(' Rest of the text.')
  })

  it('Should not fail if there is an empty string', () => {
    const input = ''
    expect(removeReasoning(input)).toBe('')
  })

  it('Should not fail if there is no reasoning text', () => {
    const input = 'Rest of the text.'
    expect(removeReasoning(input)).toBe('Rest of the text.')
  })

  it('should not fail if there is not ending tag', () => {
    const input = '<think>Reasoning text here.'
    expect(removeReasoning(input)).toBe('<think>Reasoning text here.')
  })

  it('should not fail if there is not starting tag', () => {
    const input = 'Reasoning text here.</think>'
    expect(removeReasoning(input)).toBe('Reasoning text here.</think>')
  })
})

describe('removeCitationsWithMissingLinks', () => {
  it('should remove citations with missing links', () => {
    const input = 'Citation [1] and [2] thats it[3].'
    const citationLinks = [
      'https://example.com/link1',
      'https://example.com/link2',
    ]
    expect(removeCitationsWithMissingLinks(input, citationLinks)).toBe(
      'Citation [1] and [2] thats it.',
    )
  })
})
