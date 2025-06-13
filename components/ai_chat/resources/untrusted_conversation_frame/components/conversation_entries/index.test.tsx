/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { render } from '@testing-library/react'
import '@testing-library/jest-dom'
import { useUntrustedConversationContext }
  from '../../untrusted_conversation_context'
import ConversationEntries from '.'

interface AssistantResponseProps {
  entry: Mojom.ConversationTurn
  isEntryInProgress: boolean
  allowedLinks: string[]
  isLeoModel: boolean
}

const assistantResponseMock =
  jest.fn((props: AssistantResponseProps) => <div />)

jest.mock('../assistant_response', () => ({
  __esModule: true,
  default: (props: AssistantResponseProps) => {
    assistantResponseMock(props)
    return <div />
  }
}))

jest.mock('../../untrusted_conversation_context', () => ({
  useUntrustedConversationContext: jest.fn()
}))

describe('ConversationEntries allowedLinks per response', () => {
  beforeEach(() => {
    assistantResponseMock.mockClear()

    const turn1 = {
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        { completionEvent: { completion: 'Response 1' } },
        {
          sourcesEvent: {
            sources: [{ url: { url: 'https://a.com' } }]
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
            sources: [{ url: { url: 'https://b.com' } }]
          }
        }
      ]
    }

    ;(useUntrustedConversationContext as jest.Mock).mockReturnValue({
      conversationHistory: [turn1, turn2],
      isGenerating: false,
      isMobile: false,
      isLeoModel: true,
      allModels: [],
      canSubmitUserEntries: true,
      conversationHandler: null,
      trimmedTokens: 0,
      totalTokens: 100,
      contentUsedPercentage: 100
    })
  })

  it('passes correct allowedLinks to each AssistantResponse', () => {
    render(<ConversationEntries />)
    expect(assistantResponseMock).toHaveBeenCalledTimes(2)
    expect(assistantResponseMock.mock.calls[0][0].allowedLinks).toEqual(['https://a.com'])
    expect(assistantResponseMock.mock.calls[1][0].allowedLinks).toEqual(['https://b.com'])
  })
})

describe('ConversationEntries loading spinner', () => {
  const turn1 = {
    uuid: '1',
    characterType: Mojom.CharacterType.HUMAN,
    actionType: Mojom.ActionType.UNSPECIFIED,
    text: 'Human turn',
    prompt: undefined,
    selectedText: undefined,
    events: [],
    createdTime: { internalValue: BigInt(Date.now() * 1000) },
    edits: undefined,
    uploadedFiles: undefined,
    fromBraveSearchSERP: false,
    modelKey: undefined
  }

  const turn2 = {
    ...turn1,
    uuid: '2',
    characterType: Mojom.CharacterType.ASSISTANT,
    text: 'Assistant response',
    events: [{
      completionEvent: { completion: 'Assistant response' },
      searchQueriesEvent: undefined,
      searchStatusEvent: undefined,
      sourcesEvent: undefined,
      selectedLanguageEvent: undefined,
      contentReceiptEvent: undefined,
      conversationTitleEvent: undefined
    }]
  }

  const setupContext = (history: Mojom.ConversationTurn[]) => {
    assistantResponseMock.mockClear()
    ;(useUntrustedConversationContext as jest.Mock).mockReturnValue({
      conversationHistory: history,
      isGenerating: true,
      isMobile: false,
      isLeoModel: true,
      allModels: [],
      canSubmitUserEntries: true,
      conversationHandler: null,
      trimmedTokens: 0,
      totalTokens: 100,
      contentUsedPercentage: 100
    })
    const { container } = render(<ConversationEntries />)
    const loadingSpinner = container.querySelector('leo-progressring')
    return loadingSpinner
  }

  it('shows loading spinner when human turn is'
    + 'submitted and isGenerating is true', () => {
    const loadingSpinner = setupContext([turn1])
    expect(loadingSpinner).toBeInTheDocument()
  })

  it('does not show loading spinner when completion event is present', () => {
    const loadingSpinner = setupContext([turn1, turn2])
    expect(loadingSpinner).not.toBeInTheDocument()
  })
})