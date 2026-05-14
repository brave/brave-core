// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
  getToolUseEvent,
  getWebSourcesEvent,
} from '../../../common/test_data_utils'
import useExtractTaskData from './use_extract_task_data'

describe('useExtractTaskData', () => {
  describe('splitting by completion events', () => {
    it('should create separate task items for each completion event', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getCompletionEvent('First task'),
            getToolUseEvent({
              toolName: 'tool1',
              id: '1',
              argumentsJson: '{}',
              output: undefined,
            }),
          ],
        }),
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getCompletionEvent('Second task'),
            getToolUseEvent({
              toolName: 'tool2',
              id: '2',
              argumentsJson: '{}',
              output: undefined,
            }),
          ],
        }),
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getToolUseEvent({
              toolName: 'tool3',
              id: '3',
              argumentsJson: '{}',
              output: undefined,
            }),
            getCompletionEvent('Third task'),
          ],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.taskItems).toHaveLength(3)
      expect(result.current.taskItems[0]).toHaveLength(2) // completion + tool
      expect(result.current.taskItems[1]).toHaveLength(3) // completion + tool + tool
      expect(result.current.taskItems[2]).toHaveLength(1) // completion
    })

    it('should group events before the first completion event together', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getToolUseEvent({
              toolName: 'tool1',
              id: '1',
              argumentsJson: '{}',
              output: undefined,
            }),
            getToolUseEvent({
              toolName: 'tool2',
              id: '2',
              argumentsJson: '{}',
              output: undefined,
            }),
            getCompletionEvent('First task'),
          ],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.taskItems).toHaveLength(2)
      // First task item contains events before first completion
      expect(result.current.taskItems[0]).toHaveLength(2)
      // Second task item contains the completion event
      expect(result.current.taskItems[1]).toHaveLength(1)
      expect(result.current.taskItems[1][0].completionEvent).toBeTruthy()
    })

    it('should filter out empty task items', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: undefined,
        }),
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [getCompletionEvent('Task 1')],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.taskItems).toHaveLength(1)
      expect(result.current.taskItems[0][0].completionEvent?.completion).toBe(
        'Task 1',
      )
    })
  })

  describe('extracting allowed links', () => {
    it('should extract links from sources events', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getWebSourcesEvent([
              {
                url: { url: 'https://example1.com' },
                title: 'Example 1',
                faviconUrl: { url: 'https://example1.com/favicon.ico' },
              },
              {
                url: { url: 'https://example2.com' },
                title: 'Example 2',
                faviconUrl: { url: 'https://example2.com/favicon.ico' },
              },
            ]),
            getCompletionEvent('Task'),
          ],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.allowedLinks).toHaveLength(2)
      expect(result.current.allowedLinks).toContain('https://example1.com')
      expect(result.current.allowedLinks).toContain('https://example2.com')
    })

    it('should accumulate links from multiple sources events', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getWebSourcesEvent([
              {
                url: { url: 'https://example1.com' },
                title: 'Example 1',
                faviconUrl: { url: 'https://example1.com/favicon.ico' },
              },
              {
                url: { url: 'https://example2.com' },
                title: 'Example 2',
                faviconUrl: { url: 'https://example2.com/favicon.ico' },
              },
            ]),
            getCompletionEvent('First task'),
          ],
        }),
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [
            getWebSourcesEvent([
              {
                url: { url: 'https://example2.com' },
                title: 'Duplicate Example 2',
                faviconUrl: { url: 'https://example2.com/favicon.ico' },
              },
              {
                url: { url: 'https://example3.com' },
                title: 'Example 3',
                faviconUrl: { url: 'https://example3.com/favicon.ico' },
              },
            ]),
            getCompletionEvent('Second task'),
          ],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.allowedLinks).toHaveLength(3) // ignores duplicate
      expect(result.current.allowedLinks).toContain('https://example1.com')
      expect(result.current.allowedLinks).toContain('https://example2.com')
      expect(result.current.allowedLinks).toContain('https://example3.com')
    })

    it('should handle entries with no sources events', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [getCompletionEvent('Task with no sources')],
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.allowedLinks).toHaveLength(0)
    })

    it('should handle entries with no events', () => {
      const assistantEntries: Mojom.ConversationTurn[] = [
        createConversationTurnWithDefaults({
          characterType: Mojom.CharacterType.ASSISTANT,
          events: undefined,
        }),
      ]

      const { result } = renderHook(() => useExtractTaskData(assistantEntries))

      expect(result.current.allowedLinks).toHaveLength(0)
    })
  })
})
