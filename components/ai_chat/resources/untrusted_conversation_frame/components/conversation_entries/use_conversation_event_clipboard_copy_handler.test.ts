// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
  getToolUseEvent,
  getWebSourcesEvent,
} from '../../../common/test_data_utils'
import useConversationEventClipboardCopyHandler from './use_conversation_event_clipboard_copy_handler'

const writeText = jest.fn()
Object.defineProperty(navigator, 'clipboard', {
  value: { writeText },
  configurable: true,
})

beforeEach(() => {
  writeText.mockClear()
})

describe('useConversationEventClipboardCopyHandler', () => {
  describe('human turns', () => {
    it('writes entry text directly to clipboard', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.HUMAN,
        text: 'Hello world',
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('Hello world')
    })

    it('writes empty string when text is empty', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.HUMAN,
        text: '',
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('')
    })
  })

  describe('assistant turns', () => {
    it('does not write to clipboard when there are no events', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: undefined,
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).not.toHaveBeenCalled()
    })

    it('does not write to clipboard when there is no completion event', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com' },
              title: 'Example',
              faviconUrl: { url: '' },
            },
          ]),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).not.toHaveBeenCalled()
    })

    it('falls back to entry.text when there are only tool use events', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        text: 'Result from code execution',
        events: [
          getToolUseEvent({
            toolName: 'code_execution',
            id: '1',
            argumentsJson: '{}',
          }),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('Result from code execution')
    })

    it('does not write to clipboard when there are only tool use events and no entry text', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        text: '',
        events: [
          getToolUseEvent({
            toolName: 'code_execution',
            id: '1',
            argumentsJson: '{}',
          }),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).not.toHaveBeenCalled()
    })

    it('aggregates completion text across a multi-turn group (e.g. code execution)', () => {
      const toolTurn = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        text: '',
        events: [
          getToolUseEvent({
            toolName: 'code_execution',
            id: '1',
            argumentsJson: '{}',
          }),
        ],
      })
      const completionTurn = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [getCompletionEvent('The result is 42')],
      })
      useConversationEventClipboardCopyHandler([toolTurn, completionTurn])()
      expect(writeText).toHaveBeenCalledWith('The result is 42')
    })

    it('writes completion text to clipboard', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [getCompletionEvent('Hello from assistant')],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('Hello from assistant')
    })

    it('uses the last completion event when there are multiple', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getCompletionEvent('First completion'),
          getCompletionEvent('Last completion'),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('Last completion')
    })

    it('replaces citations with URLs from sources', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com' },
              title: 'Example',
              faviconUrl: { url: '' },
            },
            {
              url: { url: 'https://other.com' },
              title: 'Other',
              faviconUrl: { url: '' },
            },
          ]),
          getCompletionEvent('See [1] and [2] for details'),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith(
        'See https://example.com and https://other.com for details',
      )
    })

    it('strips inline search blocks', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getCompletionEvent('Some text\n::search[query]{type=web}\nMore text'),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('Some text\n\nMore text')
    })

    it('strips multiple inline search blocks', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getCompletionEvent(
            '::search[query1]{type=web}\nBetween\n::search[query2]{type=image}',
          ),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('\nBetween\n')
    })

    it('applies citation replacement before stripping inline searches', () => {
      const entry = createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com' },
              title: 'Example',
              faviconUrl: { url: '' },
            },
          ]),
          getCompletionEvent('See [1]\n::search[query]{type=web}\nDone'),
        ],
      })
      useConversationEventClipboardCopyHandler([entry])()
      expect(writeText).toHaveBeenCalledWith('See https://example.com\n\nDone')
    })
  })
})
