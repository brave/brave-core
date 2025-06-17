/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { render, screen } from '@testing-library/react'
import '@testing-library/jest-dom'
import {
  defaultContext,
  UntrustedConversationContext,
  UntrustedConversationReactContext
} from '../../untrusted_conversation_context'
import ConversationEntries from '.'
import { ActionType, CharacterType, ContentType } from '../../../common/mojom'
import type { AssistantResponseProps } from '../assistant_response'

function MockDataProvider(
  props: React.PropsWithChildren<Partial<UntrustedConversationContext>>
) {
  return (
    <UntrustedConversationReactContext.Provider
      value={{
        ...defaultContext,
        ...props
      }}
    >
      {props.children}
    </UntrustedConversationReactContext.Provider>
  )
}

const assistantResponseMock = jest.fn((props: AssistantResponseProps) => <div />)

jest.mock('../assistant_response', () => ({
  __esModule: true,
  default: (props: AssistantResponseProps) => {
    assistantResponseMock(props)
    console.log('assistantResponseMock', props)
    return <div />
  }
}))

describe('ConversationEntries allowedLinks per response', () => {
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

  let mockContext: Partial<UntrustedConversationContext>

  beforeEach(() => {
    assistantResponseMock.mockClear()
    mockContext = {
      conversationHistory: [assistantTurn1, humanTurn1, assistantTurn2] as any,
      isGenerating: false,
      isMobile: false,
      isLeoModel: true,
      allModels: [],
      canSubmitUserEntries: true,
      trimmedTokens: BigInt(0),
      totalTokens: BigInt(100),
      contentUsedPercentage: 100
    }
  })

  it('passes correct allowedLinks to each AssistantResponse', () => {
    render(<MockDataProvider {...mockContext}>
      <ConversationEntries />
    </MockDataProvider>)
    expect(assistantResponseMock).toHaveBeenCalledTimes(2)
    expect(assistantResponseMock.mock.calls[0][0]?.allowedLinks).toEqual(['https://a.com'])
    expect(assistantResponseMock.mock.calls[1][0]?.allowedLinks).toEqual(['https://b.com'])
  })

  it('passes correct allowedLinks for a combined AssistantResponse group', () => {
    mockContext.conversationHistory = [humanTurn1, assistantTurn1, assistantTurn2] as any
    render(<MockDataProvider {...mockContext}>
      <ConversationEntries />
    </MockDataProvider>)
    expect(assistantResponseMock).toHaveBeenCalledTimes(2)
    expect(assistantResponseMock.mock.calls[0][0]?.allowedLinks).toEqual(['https://a.com'])
    expect(assistantResponseMock.mock.calls[1][0]?.allowedLinks).toEqual(['https://b.com'])
  })
})

describe('conversation entries', () => {
  it("doesn't render attached tabs until the conversation has started", () => {
    const { container } = render(
      <MockDataProvider
        associatedContent={[
          {
            contentId: 1,
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234'
          }
        ]}
      >
        <ConversationEntries />
      </MockDataProvider>
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]')
    ).not.toBeInTheDocument()
  })

  it('renders attached tabs on started conversation', () => {
    const { container } = render(
      <MockDataProvider
        associatedContent={[
          {
            contentId: 1,
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234'
          }
        ]}
        conversationHistory={[
          {
            characterType: CharacterType.HUMAN,
            text: 'Summarize this page',
            actionType: ActionType.SUMMARIZE_PAGE,
            events: [],
            createdTime: { internalValue: BigInt(0) },
            edits: [],
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '111',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o'
          }
        ]}
      >
        <ConversationEntries />
      </MockDataProvider>
    )

    expect(screen.getByText('Associated Content', { selector: '.title'})).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]')
    ).toBeInTheDocument()
  })

  it('only renders attached tabs on first entry', () => {
    const { container } = render(
      <MockDataProvider
        associatedContent={[
          {
            contentId: 1,
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234'
          }
        ]}
        conversationHistory={[
          {
            characterType: CharacterType.HUMAN,
            text: 'Summarize this page',
            actionType: ActionType.SUMMARIZE_PAGE,
            events: [],
            createdTime: { internalValue: BigInt(0) },
            edits: [],
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '111',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o'
          },

          {
            characterType: CharacterType.ASSISTANT,
            text: 'It Means this!',
            actionType: ActionType.RESPONSE,
            events: [],
            createdTime: { internalValue: BigInt(1) },
            edits: [],
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '222',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o'
          },
          {
            characterType: CharacterType.HUMAN,
            text: 'Question',
            actionType: ActionType.QUERY,
            events: [],
            createdTime: { internalValue: BigInt(3) },
            edits: [],
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '333',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o'
          }
        ]}
      >
        <ConversationEntries />
      </MockDataProvider>
    )

    // One for the text, one for the tooltip
    expect(screen.queryAllByText('Associated Content').length).toBe(2)
    expect(
      Array.from(container.querySelectorAll('img[src*="//favicon2"]')).length
    ).toBe(1)
  })
})
