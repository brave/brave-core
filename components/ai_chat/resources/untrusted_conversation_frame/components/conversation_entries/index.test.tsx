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

const assistantResponseMock = jest.fn(() => <div />)

jest.mock('../assistant_response', () => ({
  __esModule: true,
  default: (props: any) => {
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
