// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '$test-utils/disable_custom_elements'

import * as React from 'react'
import { act, render, waitFor } from '@testing-library/react'
import { clearAllDataForTesting } from '$web-common/api'
import { ContentType } from 'gen/brave/components/ai_chat/core/common/mojom/common.mojom.m.js'
import * as Mojom from '../../../common/mojom'
import { MockContext, MockContextProps } from '../../state/mock_context'
import TabsMenu from './attachments_menu'

// The menu's open state is gated on a transition of `query !== null`. Render
// once without input so any async data (bookmarks, history) can settle, then
// rerender with the desired `inputText` so the transition fires and the menu
// actually opens.
async function renderOpenedMenu(props: Omit<MockContextProps, 'children'>) {
  const result = render(
    <MockContext
      {...props}
      conversationOverrides={undefined}
    >
      <TabsMenu />
    </MockContext>,
  )
  // Flush any pending microtasks (async fetches like bookmarks/history) so
  // the data is ready before the menu transitions to open.
  await act(async () => {
    await Promise.resolve()
  })
  result.rerender(
    <MockContext {...props}>
      <TabsMenu />
    </MockContext>,
  )
  return result
}

describe('TabsMenu', () => {
  it('should render tabs', async () => {
    const { findByText, container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
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
      },
    })

    expect(await findByText('Test 1')).toBeInTheDocument()
    expect(await findByText('Test 2')).toBeInTheDocument()
    expect(container.querySelector('img[src*="tes1t.com"]')).toBeInTheDocument()
    expect(container.querySelector('img[src*="test2.com"]')).toBeInTheDocument()
  })

  it('should filter out attached tabs', async () => {
    const { findByText, queryByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        conversationState: {
          associatedContent: [
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
          ],
        },
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
      },
    })

    expect(await findByText('Test 2')).toBeInTheDocument()
    expect(queryByText('Test 1')).not.toBeInTheDocument()
  })

  it('should be open when query starts with @', async () => {
    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        tabs: [
          {
            contentId: 1,
            title: 'Test 1',
            url: { url: 'https://tes1t.com' },
            id: 1,
          },
        ],
      },
    })

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', true)
  })

  it('should be close when @ is removed', async () => {
    render(
      <MockContext conversationOverrides={{ inputText: ['@'] }}>
        <TabsMenu />
      </MockContext>,
    )

    const { container } = render(
      <MockContext conversationOverrides={{ inputText: ['hi'] }}>
        <TabsMenu />
      </MockContext>,
    )

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', false)
  })

  it('should filter by text after @', async () => {
    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@2'] },
      initialState: {
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
      },
    })

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('2')
    })
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
    const { queryByText, findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        tabs: [
          tab1,
          {
            contentId: 2,
            title: 'Test 2',
            url: { url: 'https://test2.com' },
            id: 2,
          },
        ],
        conversationState: {
          conversationUuid: '1',
        },
      },
      uiHandler: { associateTab },
    })

    // Wait for tabs to render and async data to load
    const item = await findByText('Test 1')
    await act(() => item.click())

    expect(associateTab).toHaveBeenCalledWith(tab1, '1')
    expect(queryByText('@')).not.toBeInTheDocument()
  })

  it('should render bookmarks in the list', async () => {
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      bookmarksService: {
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
      },
    })

    expect(await findByText('Brave Browser')).toBeInTheDocument()
    expect(await findByText('MDN Web Docs')).toBeInTheDocument()
  })

  it('should filter bookmarks by query', async () => {
    const onFetch = jest.fn()

    const { container } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@brave'] },
      bookmarksService: {
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
      },
    })

    await waitFor(() => expect(onFetch).toHaveBeenCalled())

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('Brave')
    })
  })

  it('should render history in the list', async () => {
    const { findByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      historyService: {
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
      },
    })

    expect(await findByText('Brave Search')).toBeInTheDocument()
    expect(await findByText('GitHub')).toBeInTheDocument()
  })

  it('should filter history by query', async () => {
    const onFetchHistory = jest.fn()
    const historyService = {
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
    }

    // Open the menu with a bare '@' so the history fetch can settle, then
    // rerender with the actual query so the filter applies against loaded data.
    const { container, rerender } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      historyService,
    })
    await act(async () => {
      await Promise.resolve()
    })
    rerender(
      <MockContext
        conversationOverrides={{ inputText: ['@search'] }}
        historyService={historyService}
      >
        <TabsMenu />
      </MockContext>,
    )

    // Wait for history to be queried with the new filter.
    await waitFor(() => expect(onFetchHistory).toHaveBeenCalled())

    await waitFor(() => {
      const matches = Array.from(container.querySelectorAll('.matchedText'))
      expect(matches).toHaveLength(1)
      expect(matches[0]).toHaveTextContent('Search')
    })
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

    render(
      <MockContext
        conversationOverrides={{ inputText: ['@ab'] }}
        historyService={{ getHistory }}
      >
        <TabsMenu />
      </MockContext>,
    )

    expect(getHistory).toHaveBeenCalledWith('ab', null)
  })

  it('should filter out already attached history items', async () => {
    const { findByText, queryByText } = await renderOpenedMenu({
      conversationOverrides: { inputText: ['@'] },
      initialState: {
        conversationState: {
          associatedContent: [
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
          ],
        },
      },
      historyService: {
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
      },
    })

    expect(await findByText('GitHub')).toBeInTheDocument()
    expect(queryByText('Brave Search')).not.toBeInTheDocument()
  })
})
