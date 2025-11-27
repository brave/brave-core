// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import {
  AIChatContext,
  AIChatReactContext,
  defaultContext as defaultAIChatContext,
} from '../../state/ai_chat_context'
import {
  ConversationReactContext,
  ConversationContext,
  defaultContext as defaultConversationContext,
} from '../../state/conversation_context'
import { act, render } from '@testing-library/react'
import TabsMenu from './attachments_menu'
import { ContentType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'

const MockContext = (
  props: React.PropsWithChildren<Partial<AIChatContext & ConversationContext>>,
) => {
  return (
    <AIChatReactContext.Provider
      value={{
        ...defaultAIChatContext,
        ...props,
      }}
    >
      <ConversationReactContext.Provider
        value={{
          ...defaultConversationContext,
          ...props,
        }}
      >
        {props.children}
      </ConversationReactContext.Provider>
    </AIChatReactContext.Provider>
  )
}

describe('TabsMenu', () => {
  it('should render tabs', () => {
    const { getByText, container } = render(
      <MockContext
        tabs={[
          {
            contentId: 1,
            title: 'Test 1',
            url: {
              url: 'https://tes1t.com',
            },
            id: 1,
          },
          {
            contentId: 2,
            title: 'Test 2',
            url: {
              url: 'https://test2.com',
            },
            id: 2,
          },
        ]}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getByText('Test 1')).toBeInTheDocument()
    expect(getByText('Test 2')).toBeInTheDocument()
    expect(container.querySelector('img[src*="tes1t.com"]')).toBeInTheDocument()
    expect(container.querySelector('img[src*="test2.com"]')).toBeInTheDocument()
  })

  it('should filter out attached tabs', () => {
    const { queryByText } = render(
      <MockContext
        associatedContentInfo={[
          {
            conversationTurnUuid: '1',
            contentId: 1,
            title: 'Test 1',
            url: {
              url: 'https://test1.com',
            },
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0,
            uuid: '1',
          },
        ]}
        tabs={[
          {
            contentId: 1,
            title: 'Test 1',
            url: {
              url: 'https://tes1t.com',
            },
            id: 1,
          },
          {
            contentId: 2,
            title: 'Test 2',
            url: {
              url: 'https://test2.com',
            },
            id: 2,
          },
        ]}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(queryByText('Test 1')).not.toBeInTheDocument()
    expect(queryByText('Test 2')).toBeInTheDocument()
  })

  it('should be open when query starts with @', () => {
    const { container } = render(
      <MockContext inputText={['@']}>
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', true)
  })

  it('should be close when @ is removed', () => {
    render(
      <MockContext inputText={['@']}>
        <TabsMenu />
      </MockContext>,
    )

    const { container } = render(
      <MockContext inputText={['hi']}>
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', false)
  })

  it('should filter by text after @', async () => {
    const { container } = await act(async () =>
      render(
        <MockContext
          inputText={['@2']}
          tabs={[
            {
              contentId: 1,
              title: 'Test 1',
              url: {
                url: 'https://tes1t.com',
              },
              id: 1,
            },
            {
              contentId: 2,
              title: 'Test 2',
              url: {
                url: 'https://test2.com',
              },
              id: 2,
            },
          ]}
        >
          <TabsMenu />
        </MockContext>,
      ),
    )

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('2')
  })

  it('selecting an element should clear text and attempt to associate with current conversation', () => {
    const associateTab = jest.fn()
    const tab1 = {
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1,
    }
    const { queryByText } = render(
      <MockContext
        conversationUuid='1'
        inputText={['@']}
        uiHandler={
          // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
          {
            associateTab,
            ...defaultAIChatContext.uiHandler,
          } as any
        }
        tabs={[
          tab1,
          {
            contentId: 2,
            title: 'Test 2',
            url: {
              url: 'https://test2.com',
            },
            id: 2,
          },
        ]}
      >
        <TabsMenu />
      </MockContext>,
    )

    queryByText('Test 1')?.click()

    expect(associateTab).toHaveBeenCalledWith(tab1, '1')
    expect(queryByText('@')).not.toBeInTheDocument()
  })

  it('should render bookmarks in the list', async () => {
    const { findByText } = render(
      <MockContext
        getBookmarks={() =>
          Promise.resolve([
            {
              id: BigInt(1),
              title: 'Brave Browser',
              url: { url: 'https://brave.com' },
            },
            {
              id: BigInt(2),
              title: 'MDN Web Docs',
              url: { url: 'https://developer.mozilla.org' },
            },
          ])
        }
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(await findByText('Brave Browser')).toBeInTheDocument()
    expect(await findByText('MDN Web Docs')).toBeInTheDocument()
  })

  it('should filter bookmarks by query', async () => {
    const { container } = await act(async () =>
      render(
        <MockContext
          inputText={['@brave']}
          getBookmarks={() =>
            Promise.resolve([
              {
                id: BigInt(1),
                title: 'Brave Browser',
                url: { url: 'https://brave.com' },
              },
              {
                id: BigInt(2),
                title: 'MDN Web Docs',
                url: { url: 'https://developer.mozilla.org' },
              },
            ])
          }
        >
          <TabsMenu />
        </MockContext>,
      ),
    )

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('Brave')
  })

  it('should render history in the list', async () => {
    const { findByText } = render(
      <MockContext
        getHistory={() =>
          Promise.resolve([
            {
              id: BigInt(1),
              title: 'Brave Search',
              url: { url: 'https://search.brave.com' },
            },
            {
              id: BigInt(2),
              title: 'GitHub',
              url: { url: 'https://github.com' },
            },
          ])
        }
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(await findByText('Brave Search')).toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })

  it('should filter history by query', async () => {
    const { container } = await act(async () =>
      render(
        <MockContext
          inputText={['@search']}
          getHistory={() =>
            Promise.resolve([
              {
                id: BigInt(1),
                title: 'Brave Search',
                url: { url: 'https://search.brave.com' },
              },
              {
                id: BigInt(2),
                title: 'GitHub',
                url: { url: 'https://github.com' },
              },
            ])
          }
        >
          <TabsMenu />
        </MockContext>,
      ),
    )

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('Search')
  })

  it('should pass query to getHistory when query length >= 2', async () => {
    const getHistory = jest.fn(() =>
      Promise.resolve([
        {
          id: BigInt(1),
          title: 'Test History',
          url: { url: 'https://test.com' },
        },
      ]),
    )

    render(
      <MockContext
        inputText={['@ab']}
        getHistory={getHistory}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getHistory).toHaveBeenCalledWith('ab')
  })

  it('should filter out already attached history items', async () => {
    const { findByText, queryByText } = render(
      <MockContext
        associatedContentInfo={[
          {
            conversationTurnUuid: undefined,
            contentId: 1,
            title: 'Brave Search',
            url: {
              url: 'https://search.brave.com',
            },
            contentType: ContentType.PageContent,
            contentUsedPercentage: 0,
            uuid: '1',
          },
        ]}
        getHistory={() =>
          Promise.resolve([
            {
              id: BigInt(1),
              title: 'Brave Search',
              url: { url: 'https://search.brave.com' },
            },
            {
              id: BigInt(2),
              title: 'GitHub',
              url: { url: 'https://github.com' },
            },
          ])
        }
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(queryByText('Brave Search')).not.toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })
})
