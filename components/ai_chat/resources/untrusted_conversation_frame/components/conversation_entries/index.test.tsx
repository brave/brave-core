// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import ConversationEntries from '.'
import {
  defaultContext,
  UntrustedConversationContext,
  UntrustedConversationReactContext
} from '../../untrusted_conversation_context'
import { ActionType, CharacterType, ContentType } from '../../../common/mojom'
;(StyleSheet as any).prototype.insertRule = (...args: any[]) => {
  console.log(args)
}

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

describe('conversation entries', () => {
  it("doesn't render attached tabs until the conversation has started", () => {
    const { container } = render(
      <MockDataProvider
        associatedContent={[
          {
            contentId: 1,
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0.5,
            isContentRefined: false,
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
            isContentRefined: false,
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
            selectedText: undefined
          }
        ]}
      >
        <ConversationEntries />
      </MockDataProvider>
    )

    expect(screen.queryByText('Associated Content')).toBeInTheDocument()
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
            isContentRefined: false,
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
            selectedText: undefined
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
            selectedText: undefined
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
            selectedText: undefined
          }
        ]}
      >
        <ConversationEntries />
      </MockDataProvider>
    )

    expect(screen.queryAllByText('Associated Content').length).toBe(1)
    expect(
      Array.from(container.querySelectorAll('img[src*="//favicon2"]')).length
    ).toBe(1)
  })
})
