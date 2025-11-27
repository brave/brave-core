/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { fireEvent, render, screen } from '@testing-library/react'
import '@testing-library/jest-dom'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import { getCompletionEvent } from '../../../common/test_data_utils'
import MockContext from '../../mock_untrusted_conversation_context'
import { UntrustedConversationContext } from '../../untrusted_conversation_context'
import type { AssistantResponseProps } from '../assistant_response'
import ConversationEntries from '.'

const assistantResponseMock = jest.fn((props: AssistantResponseProps) => (
  <div />
))

jest.mock('../assistant_response', () => ({
  __esModule: true,
  default: (props: AssistantResponseProps) => {
    assistantResponseMock(props)
    return <div />
  },
}))

describe('ConversationEntries allowedLinks per response', () => {
  const assistantTurn1 = {
    characterType: Mojom.CharacterType.ASSISTANT,
    events: [
      { completionEvent: { completion: 'Response 1' } },
      {
        sourcesEvent: {
          sources: [
            {
              url: { url: 'https://a.com' },
              title: 'Title 1',
              faviconUrl: { url: 'https://a.com/favicon.ico' },
            },
          ],
        },
      },
    ],
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
          sources: [
            {
              url: { url: 'https://b.com' },
              title: 'Title 2',
              faviconUrl: { url: 'https://b.com/favicon.ico' },
            },
          ],
        },
      },
    ],
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
      contentUsedPercentage: 100,
    }
  })

  it('passes correct allowedLinks to each AssistantResponse', () => {
    render(
      <MockContext {...mockContext}>
        <ConversationEntries />
      </MockContext>,
    )
    expect(assistantResponseMock).toHaveBeenCalledTimes(2)
    expect(assistantResponseMock.mock.calls[0][0]?.allowedLinks).toEqual([
      'https://a.com',
    ])
    expect(assistantResponseMock.mock.calls[1][0]?.allowedLinks).toEqual([
      'https://b.com',
    ])
  })

  it('passes correct allowedLinks for a combined AssistantResponse group', () => {
    mockContext.conversationHistory = [
      humanTurn1,
      assistantTurn1,
      assistantTurn2,
    ] as any
    render(
      <MockContext {...mockContext}>
        <ConversationEntries />
      </MockContext>,
    )
    expect(assistantResponseMock).toHaveBeenCalledTimes(2)
    expect(assistantResponseMock.mock.calls[0][0]?.allowedLinks).toEqual([
      'https://a.com',
    ])
    expect(assistantResponseMock.mock.calls[1][0]?.allowedLinks).toEqual([
      'https://b.com',
    ])
  })
})

describe('conversation entries', () => {
  it("doesn't render attached tabs until the conversation has started", () => {
    const { container } = render(
      <MockContext
        associatedContent={[
          {
            contentId: 1,
            contentType: Mojom.ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234',
            conversationTurnUuid: '111',
          },
        ]}
      >
        <ConversationEntries />
      </MockContext>,
    )

    expect(screen.queryByText('Associated Content')).not.toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).not.toBeInTheDocument()
  })

  it('renders attached tabs on started conversation', () => {
    const { container } = render(
      <MockContext
        associatedContent={[
          {
            contentId: 1,
            contentType: Mojom.ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234',
            conversationTurnUuid: '111',
          },
        ]}
        conversationHistory={[
          {
            characterType: Mojom.CharacterType.HUMAN,
            text: 'Summarize this page',
            actionType: Mojom.ActionType.SUMMARIZE_PAGE,
            events: [],
            createdTime: { internalValue: BigInt(0) },
            edits: [],
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '111',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o',
          },
        ]}
      >
        <ConversationEntries />
      </MockContext>,
    )

    expect(
      screen.getByText('Associated Content', { selector: '.title' }),
    ).toBeInTheDocument()
    expect(
      container.querySelector('img[src*="//favicon2"]'),
    ).toBeInTheDocument()
  })

  it('only renders attached tabs on first entry', () => {
    const { container } = render(
      <MockContext
        associatedContent={[
          {
            contentId: 1,
            contentType: Mojom.ContentType.PageContent,
            contentUsedPercentage: 0.5,
            title: 'Associated Content',
            url: { url: 'https://example.com' },
            uuid: '1234',
            conversationTurnUuid: '111',
          },
        ]}
        conversationHistory={[
          {
            characterType: Mojom.CharacterType.HUMAN,
            text: 'Summarize this page',
            actionType: Mojom.ActionType.SUMMARIZE_PAGE,
            events: undefined,
            createdTime: { internalValue: BigInt(0) },
            edits: undefined,
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '111',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o',
          },

          {
            characterType: Mojom.CharacterType.ASSISTANT,
            text: 'It Means this!',
            actionType: Mojom.ActionType.RESPONSE,
            events: undefined,
            createdTime: { internalValue: BigInt(1) },
            edits: undefined,
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '222',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o',
          },
          {
            characterType: Mojom.CharacterType.HUMAN,
            text: 'Question',
            actionType: Mojom.ActionType.QUERY,
            events: undefined,
            createdTime: { internalValue: BigInt(3) },
            edits: undefined,
            fromBraveSearchSERP: false,
            uploadedFiles: [],
            uuid: '333',
            prompt: undefined,
            selectedText: undefined,
            modelKey: 'gpt-4o',
          },
        ]}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // One for the text, one for the tooltip
    expect(screen.queryAllByText('Associated Content').length).toBe(1)
    expect(
      Array.from(container.querySelectorAll('img[src*="//favicon2"]')).length,
    ).toBe(1)
  })

  test('allows editing of entries', () => {
    const humanTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.HUMAN,
      text: 'What is the meaning of life?',
      actionType: Mojom.ActionType.QUERY,
      createdTime: { internalValue: BigInt(0) },
      events: undefined,
      edits: [],
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '111',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'gpt-4o',
    }

    const assistantTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.ASSISTANT,
      text: 'assistant never renders text property',
      actionType: Mojom.ActionType.RESPONSE,
      createdTime: { internalValue: BigInt(4) },
      events: [getCompletionEvent('It Means this!')],
      edits: undefined,
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '444',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'any-model',
    }

    render(
      <MockContext
        conversationHistory={[humanTurn1, assistantTurn1]}
        canSubmitUserEntries={true}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should show the edit button when hovering over the human turn
    fireEvent.mouseEnter(screen.getByText('What is the meaning of life?'))
    expect(screen.queryByTitle(S.CHAT_UI_EDIT_BUTTON_LABEL)).toBeInTheDocument()
    // should show the assistant edit button
    expect(
      screen.queryByText(S.CHAT_UI_EDIT_PROMPT_BUTTON_LABEL),
    ).toBeInTheDocument()
  })

  test('disallows editing of entries for agent conversations', () => {
    const humanTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.HUMAN,
      text: 'What is the meaning of life?',
      actionType: Mojom.ActionType.QUERY,
      createdTime: { internalValue: BigInt(0) },
      events: undefined,
      edits: [],
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '111',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'gpt-4o',
    }

    const assistantTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.ASSISTANT,
      text: 'assistant never renders text property',
      actionType: Mojom.ActionType.RESPONSE,
      createdTime: { internalValue: BigInt(4) },
      events: [getCompletionEvent('It Means this!')],
      edits: undefined,
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '444',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'any-model',
    }

    render(
      <MockContext
        conversationHistory={[humanTurn1, assistantTurn1]}
        conversationCapability={Mojom.ConversationCapability.CONTENT_AGENT}
        canSubmitUserEntries={true}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should not show the edit buttons
    fireEvent.mouseEnter(screen.getByText('What is the meaning of life?'))
    expect(
      screen.queryByTitle(S.CHAT_UI_EDIT_BUTTON_LABEL),
    ).not.toBeInTheDocument()
    expect(
      screen.queryByText(S.CHAT_UI_EDIT_PROMPT_BUTTON_LABEL),
    ).not.toBeInTheDocument()
  })

  test('displays the edited version of entries', () => {
    const humanTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.HUMAN,
      text: 'What is the meaning of life?',
      actionType: Mojom.ActionType.QUERY,
      createdTime: { internalValue: BigInt(0) },
      events: undefined,
      edits: [],
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '111',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'gpt-4o',
    }

    humanTurn1.edits = [
      {
        ...humanTurn1,
        uuid: '222',
        text: 'What is the meaning of edits?',
        createdTime: { internalValue: BigInt(2) },
      },
      {
        ...humanTurn1,
        uuid: '333',
        text: 'What is the meaning of the current edit?',
        createdTime: { internalValue: BigInt(1) },
      },
    ]

    const assistantTurn1: Mojom.ConversationTurn = {
      characterType: Mojom.CharacterType.ASSISTANT,
      text: 'assistant never renders text property',
      actionType: Mojom.ActionType.RESPONSE,
      createdTime: { internalValue: BigInt(4) },
      events: [getCompletionEvent('It Means this!')],
      edits: undefined,
      fromBraveSearchSERP: false,
      uploadedFiles: [],
      uuid: '444',
      prompt: undefined,
      selectedText: undefined,
      modelKey: 'any-model',
    }

    assistantTurn1.edits = [
      {
        ...assistantTurn1,
        uuid: '555',
        text: 'assistant never renders text property',
        events: [getCompletionEvent('It Means edits!')],
        createdTime: { internalValue: BigInt(5) },
      },
      {
        ...assistantTurn1,
        uuid: '666',
        text: 'assistant never renders text property',
        events: [getCompletionEvent('It Means the current edit!')],
        createdTime: { internalValue: BigInt(6) },
      },
    ]

    render(
      <MockContext conversationHistory={[humanTurn1, assistantTurn1]}>
        <ConversationEntries />
      </MockContext>,
    )

    // Should show the edited version of the entries
    expect(
      screen.queryByText('What is the meaning of the current edit?'),
    ).toBeInTheDocument()
    expect(
      screen.queryByText('What is the meaning of life?'),
    ).not.toBeInTheDocument()
    expect(
      screen.queryByText('What is the meaning of edits?'),
    ).not.toBeInTheDocument()
    expect(
      screen.queryByText('assistant never renders text property'),
    ).not.toBeInTheDocument()

    expect(assistantResponseMock).toHaveBeenCalledTimes(1)
    expect(assistantResponseMock.mock.calls[0][0]?.events).toEqual([
      getCompletionEvent('It Means the current edit!'),
    ])
  })
})

describe('ConversationEntries visualContentUsedPercentage handling', () => {
  const createAssistantTurn = (): Partial<Mojom.ConversationTurn> => ({
    characterType: Mojom.CharacterType.ASSISTANT,
    events: [{ completionEvent: { completion: 'Assistant response' } }],
  })

  const createHumanTurn = (
    overrides: Partial<Mojom.ConversationTurn> = {},
  ): Mojom.ConversationTurn =>
    ({
      characterType: Mojom.CharacterType.HUMAN,
      text: 'Human question',
      ...overrides,
    }) as Mojom.ConversationTurn

  beforeEach(() => {
    assistantResponseMock.mockClear()
  })

  test('should show visual content warning when visualContentUsedPercentage is less than 100', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={75}
        contentUsedPercentage={100}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should render LongVisualContentWarning component when visual content is truncated
    // We check for the info icon which is part of the warning components
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).toBeInTheDocument()
  })

  test('should show visual content warning when visualContentUsedPercentage is 0', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={0}
        contentUsedPercentage={100}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).toBeInTheDocument()
  })

  test('should not show content warning when visualContentUsedPercentage is 100', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={100}
        contentUsedPercentage={100}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should not show warning when no content truncation
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).not.toBeInTheDocument()
  })

  test('should not show content warning when visualContentUsedPercentage is undefined (defaults to 100)', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={undefined}
        contentUsedPercentage={100}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should not show warning when visualContentUsedPercentage is undefined
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).not.toBeInTheDocument()
  })

  test('should show visual content warning when both contentUsedPercentage and visualContentUsedPercentage are truncated', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={60}
        contentUsedPercentage={80}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should show visual content warning (prioritizes visual content warning over page content warning)
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).toBeInTheDocument()
  })

  test('should show text content warning when visualContentUsedPercentage is fine but trimmed tokens exist', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn(), createAssistantTurn()]}
        visualContentUsedPercentage={100}
        contentUsedPercentage={100}
        trimmedTokens={BigInt(200)}
        totalTokens={BigInt(1000)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should show text content warning due to trimmed tokens (highest priority)
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).toBeInTheDocument()
  })

  test('should only show content warnings for assistant responses, not human turns', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[createHumanTurn()]} // Only human turn
        visualContentUsedPercentage={50}
        contentUsedPercentage={80}
        trimmedTokens={BigInt(0)}
        totalTokens={BigInt(0)}
      >
        <ConversationEntries />
      </MockContext>,
    )

    // Should not show warnings for human-only conversation
    const infoElement = container.querySelector('leo-icon[name="info-outline"]')
    expect(infoElement).not.toBeInTheDocument()
  })

  test('should highlight skill shortcuts in text', () => {
    const { container } = render(
      <MockContext
        conversationHistory={[
          createHumanTurn({
            text: '/shortcut and additional text',
            uuid: '111',
            skill: { shortcut: 'shortcut', prompt: 'prompt' },
          }),
        ]}
      >
        <ConversationEntries />
      </MockContext>,
    )
    expect(container.querySelector('.skillLabel')).toBeInTheDocument()
    expect(container.querySelector('.skillLabel')).toHaveTextContent(
      '/shortcut',
    )
    expect(container.querySelector('.skillLabel')).not.toHaveTextContent(
      'and additional text',
    )
  })
})
