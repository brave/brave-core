// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import {
  ConversationReactContext,
  ConversationContext,
  defaultContext as defaultConversationContext,
} from '../../state/conversation_context'
import { act, render, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { ContentType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import * as Mojom from '../../../common/mojom'
import {
  MockContext as AIChatMockContext,
  MockContextProps,
} from '../../state/mock_context'
import TabsMenu from './attachments_menu'

const MockContext = (
  props: React.PropsWithChildren<
    Partial<MockContextProps & ConversationContext>
  >,
) => {
  const { children, ...contextOverrides } = props

  return (
    <AIChatMockContext {...contextOverrides}>
      <ConversationReactContext.Provider
        value={{
          ...defaultConversationContext,
          ...contextOverrides,
        }}
      >
        {children}
      </ConversationReactContext.Provider>
    </AIChatMockContext>
  )
}

// Render and flush async state updates from usePromise hooks.
async function renderTabsMenu(
  ...args: Parameters<typeof render>
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(...args)
  })
  return result!
}

describe('TabsMenu', () => {
  // Clear the shared QueryClient between tests to avoid cache pollution
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('should render tabs', async () => {
    const { getByText, container } = await renderTabsMenu(
      <MockContext
        initialState={{
          tabs: [
            {
              contentId: 1,
              title: 'Test 1',
              url: { url: 'https://tes1t.com' },
              id: 1,
            },
            {
              contentId: 2,
              title: 'Test 2',
              url: { url: 'https://test2.com' },
              id: 2,
            },
          ],
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getByText('Test 1')).toBeInTheDocument()
    expect(getByText('Test 2')).toBeInTheDocument()
    expect(container.querySelector('img[src*="tes1t.com"]')).toBeInTheDocument()
    expect(container.querySelector('img[src*="test2.com"]')).toBeInTheDocument()
  })

  it('should filter out attached tabs', async () => {
    const { queryByText } = await renderTabsMenu(
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
        initialState={{
          tabs: [
            {
              contentId: 1,
              title: 'Test 1',
              url: { url: 'https://tes1t.com' },
              id: 1,
            },
            {
              contentId: 2,
              title: 'Test 2',
              url: { url: 'https://test2.com' },
              id: 2,
            },
          ],
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(queryByText('Test 1')).not.toBeInTheDocument()
    expect(queryByText('Test 2')).toBeInTheDocument()
  })

  it('should be open when query starts with @', async () => {
    const { container } = await renderTabsMenu(
      <MockContext
        inputText={['@']}
        initialState={{
          tabs: [
            {
              contentId: 1,
              title: 'Test 1',
              url: { url: 'https://tes1t.com' },
              id: 1,
            },
          ],
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', true)
  })

  it('should be close when @ is removed', async () => {
    await renderTabsMenu(
      <MockContext inputText={['@']}>
        <TabsMenu />
      </MockContext>,
    )

    const { container } = await renderTabsMenu(
      <MockContext inputText={['hi']}>
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', false)
  })

  it('should filter by text after @', async () => {
    const { container } = await act(
      async () =>
        await renderTabsMenu(
          <MockContext
            inputText={['@2']}
            initialState={{
              tabs: [
                {
                  contentId: 1,
                  title: 'Test 1',
                  url: { url: 'https://tes1t.com' },
                  id: 1,
                },
                {
                  contentId: 2,
                  title: 'Test 2',
                  url: { url: 'https://test2.com' },
                  id: 2,
                },
              ],
            }}
          >
            <TabsMenu />
          </MockContext>,
        ),
    )

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('2')
  })

  it('selecting an element should clear text and attempt to associate with current conversation', async () => {
    const associateTab = jest.fn()
    const tab1 = {
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1,
    }
    const { queryByText, findByText } = await renderTabsMenu(
      <MockContext
        conversationUuid='1'
        inputText={['@']}
        initialState={{
          tabs: [
            tab1,
            {
              contentId: 2,
              title: 'Test 2',
              url: { url: 'https://test2.com' },
              id: 2,
            },
          ],
        }}
        uiHandler={{ associateTab }}
      >
        <TabsMenu />
      </MockContext>,
    )

    // Wait for tabs to render and async data to load
    const item = await findByText('Test 1')
    await act(() => item.click())

    expect(associateTab).toHaveBeenCalledWith(tab1, '1')
    expect(queryByText('@')).not.toBeInTheDocument()
  })

  it('should render bookmarks in the list', async () => {
    const { findByText } = await renderTabsMenu(
      <MockContext
        bookmarksService={{
          getBookmarks: () =>
            Promise.resolve({
              bookmarks: [
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
              ],
            }),
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(await findByText('Brave Browser')).toBeInTheDocument()
    expect(await findByText('MDN Web Docs')).toBeInTheDocument()
  })

  it('should filter bookmarks by query', async () => {
    const onFetch = jest.fn()

    const { container } = await renderTabsMenu(
      <MockContext
        inputText={['@brave']}
        bookmarksService={{
          getBookmarks: () => {
            onFetch()
            return Promise.resolve({
              bookmarks: [
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
              ],
            })
          },
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    await act(() => waitFor(() => expect(onFetch).toHaveBeenCalled()))

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('Brave')
  })

  it('should render history in the list', async () => {
    const { findByText } = await renderTabsMenu(
      <MockContext
        historyService={{
          getHistory: () =>
            Promise.resolve({
              history: [
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
              ],
            }),
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(await findByText('Brave Search')).toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })

  it('should filter history by query', async () => {
    const onFetchHistory = jest.fn()

    const { container } = await renderTabsMenu(
      <MockContext
        inputText={['@search']}
        historyService={{
          getHistory: () => {
            onFetchHistory()
            return Promise.resolve({
              history: [
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
              ],
            })
          },
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    // Wait for history to be queried
    await act(() => waitFor(() => expect(onFetchHistory).toHaveBeenCalled()))

    const matches = Array.from(container.querySelectorAll('.matchedText'))
    expect(matches).toHaveLength(1)
    expect(matches[0]).toHaveTextContent('Search')
  })

  it('should pass query to getHistory when query length >= 2', async () => {
    const getHistory = jest.fn(() =>
      Promise.resolve({
        history: [
          {
            id: BigInt(1),
            title: 'Test History',
            url: { url: 'https://test.com' },
          },
        ] satisfies Mojom.HistoryEntry[],
      }),
    )

    await renderTabsMenu(
      <MockContext
        inputText={['@ab']}
        historyService={{ getHistory }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getHistory).toHaveBeenCalledWith('ab', null)
  })

  it('should filter out already attached history items', async () => {
    const { findByText, queryByText } = await renderTabsMenu(
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
        historyService={{
          getHistory: () =>
            Promise.resolve({
              history: [
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
              ],
            }),
        }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(queryByText('Brave Search')).not.toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })
})
