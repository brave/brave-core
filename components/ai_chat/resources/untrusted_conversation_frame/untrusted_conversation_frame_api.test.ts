// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../common/mojom'
import { createTextContentBlock } from '../common/content_block'
import { updateToolUseEventInHistory } from './api/untrusted_conversation_api'

describe('updateToolUseEventInHistory', () => {
  const mockToolUseEvent: Mojom.ToolUseEvent = {
    toolName: 'test_tool',
    id: 'tool-123',
    argumentsJson: '{"param": "value"}',
    output: [createTextContentBlock('Updated output')],
  }

  const createMockConversationTurn = (
    uuid: string,
    events?: Mojom.ConversationEntryEvent[],
  ): Mojom.ConversationTurn => ({
    uuid,
    characterType: Mojom.CharacterType.ASSISTANT,
    actionType: Mojom.ActionType.RESPONSE,
    text: 'Mock conversation turn',
    // @ts-expect-error jest doesn't support bigint for json serialization
    createdTime: { internalValue: 1 },
    events,
    prompt: 'Mock prompt',
    selectedText: 'Mock selected text',
    edits: undefined,
    uploadedFiles: [],
    fromBraveSearchSERP: false,
    modelKey: 'mock-model',
  })

  const createMockToolUseEventEntry = (
    toolUseId: string,
  ): Mojom.ConversationEntryEvent =>
    ({
      toolUseEvent: {
        toolName: 'existing_tool',
        id: toolUseId,
        argumentsJson: '{"old": "data"}',
        output: undefined,
      },
    }) as Mojom.ConversationEntryEvent

  const createMockCompletionEventEntry = (
    completion: string,
  ): Mojom.ConversationEntryEvent =>
    ({
      completionEvent: { completion },
    }) as Mojom.ConversationEntryEvent

  describe('successful updates', () => {
    test('should update tool use event when entry and event are found', () => {
      const mockEvents = [
        createMockCompletionEventEntry('Some text'),
        createMockToolUseEventEntry('tool-123'),
        createMockCompletionEventEntry('More text'),
      ]

      const history = [
        createMockConversationTurn('entry-1', mockEvents),
        createMockConversationTurn('entry-2'),
      ]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).not.toBeNull()
      expect(result).toHaveLength(2)
      expect(result![0].uuid).toBe('entry-1')
      expect(result![0].events).toHaveLength(3)
      expect(result![0].events![1].toolUseEvent).toEqual(mockToolUseEvent)
      expect(result![0].events![1].toolUseEvent).not.toEqual(
        mockEvents[1].toolUseEvent,
      )

      // Verify other events are unchanged
      expect(result![0].events![0]).toEqual(mockEvents[0])
      expect(result![0].events![2]).toEqual(mockEvents[2])

      // Verify original history is not mutated
      expect(history[0].events![1].toolUseEvent!.output).toBeUndefined()
    })

    test('should update tool use event in first position', () => {
      const mockEvents = [
        createMockToolUseEventEntry('tool-123'),
        createMockToolUseEventEntry('tool-456'),
      ]
      const history = [createMockConversationTurn('entry-1', mockEvents)]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).not.toBeNull()
      expect(result![0].events![0].toolUseEvent).toEqual(mockToolUseEvent)
      expect(result![0].events![1].toolUseEvent).toEqual(
        mockEvents[1].toolUseEvent,
      )
    })

    test('should update tool use event in last position', () => {
      const mockEvents = [
        createMockToolUseEventEntry('tool-456'),
        createMockCompletionEventEntry('Text'),
        createMockToolUseEventEntry('tool-123'),
      ]
      const history = [createMockConversationTurn('entry-1', mockEvents)]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).not.toBeNull()
      expect(result![0].events![2].toolUseEvent).toEqual(mockToolUseEvent)
    })
  })

  describe('failure cases', () => {
    test('should return null when entry UUID is not found', () => {
      const history = [
        createMockConversationTurn('entry-1'),
        createMockConversationTurn('entry-2'),
      ]

      const result = updateToolUseEventInHistory(
        history,
        'non-existent',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should return null when entry has no events', () => {
      const history = [createMockConversationTurn('entry-1')]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should return null when entry has empty events array', () => {
      const history = [createMockConversationTurn('entry-1', [])]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should return null when no matching tool use event ID is found', () => {
      const mockEvents = [
        createMockCompletionEventEntry('Some text'),
        createMockToolUseEventEntry('different-tool-id'),
      ]
      const history = [createMockConversationTurn('entry-1', mockEvents)]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should return null when events contain non-tool-use events only', () => {
      const mockEvents = [createMockCompletionEventEntry('Some text')]
      const history = [createMockConversationTurn('entry-1', mockEvents)]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })
  })

  describe('edge cases', () => {
    test('should handle empty history', () => {
      const result = updateToolUseEventInHistory(
        [],
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should handle history with entries having undefined UUID', () => {
      const history = [
        { ...createMockConversationTurn('entry-1'), uuid: undefined },
        createMockConversationTurn('entry-2'),
      ]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).toBeNull()
    })

    test('should not mutate the original history array', () => {
      const mockEvents = [createMockToolUseEventEntry('tool-123')]
      const history = [createMockConversationTurn('entry-1', mockEvents)]
      const originalHistory = JSON.parse(JSON.stringify(history))

      updateToolUseEventInHistory(history, 'entry-1', mockToolUseEvent)

      expect(history).toEqual(originalHistory)
    })

    test('should handle multiple entries with same tool use event ID', () => {
      // Should only update the first matching entry
      const mockEvents1 = [createMockToolUseEventEntry('tool-123')]
      const mockEvents2 = [createMockToolUseEventEntry('tool-123')]

      const history = [
        createMockConversationTurn('entry-1', mockEvents1),
        createMockConversationTurn('entry-2', mockEvents2),
      ]

      const result = updateToolUseEventInHistory(
        history,
        'entry-1',
        mockToolUseEvent,
      )

      expect(result).not.toBeNull()
      expect(result![0].events![0].toolUseEvent).toEqual(mockToolUseEvent)
      // Second entry should remain unchanged
      expect(result![1].events![0].toolUseEvent!.argumentsJson).toBe(
        '{"old": "data"}',
      )
    })
  })
})
