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
})
