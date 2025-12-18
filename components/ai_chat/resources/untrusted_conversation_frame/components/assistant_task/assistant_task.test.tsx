// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { act, render, screen, waitFor } from '@testing-library/react'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
  getSearchQueriesEvent,
  getToolUseEvent,
  getWebSourcesEvent,
  getEventTemplate,
} from '../../../common/test_data_utils'
import MockContext, {
  MockContextRef,
} from '../../mock_untrusted_conversation_context'
import AssistantTask from './assistant_task'

describe('AssistantTask', () => {
  const mockAssistantEntries: Mojom.ConversationTurn[] = [
    createConversationTurnWithDefaults({
      uuid: 'test-uuid',
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [getCompletionEvent('Task in progress')],
    }),
  ]

  test('should display thumbnail when contentTaskTabId is set and task is active', async () => {
    const addTabToThumbnailTracker = jest.fn()
    const removeTabFromThumbnailTracker = jest.fn()
    const testTabId = 123
    const testDataURI =
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='

    const mockRef = React.createRef<MockContextRef>()

    render(
      <MockContext
        ref={mockRef}
        uiHandler={{
          addTabToThumbnailTracker,
          removeTabFromThumbnailTracker,
        }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Emit contentTaskStarted to set the contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Verify thumbnail tracker was called
    await waitFor(() => {
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(testTabId)
    })

    // Simulate thumbnail update
    await act(async () => {
      mockRef.current!.api.emitEvent('thumbnailUpdated', [
        testTabId,
        testDataURI,
      ])
    })

    const img = screen.getByRole('img')
    expect(img).toBeInTheDocument()
    expect(img).toHaveAttribute('src', testDataURI)
  })

  test('should not display thumbnail when contentTaskTabId is not set', async () => {
    const addTabToThumbnailTracker = jest.fn()

    render(
      <MockContext uiHandler={{ addTabToThumbnailTracker }}>
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify no tracker setup when no contentTaskStarted event emitted
    expect(addTabToThumbnailTracker).not.toHaveBeenCalled()

    // Verify no image is rendered
    expect(screen.queryByRole('img')).not.toBeInTheDocument()
  })

  test('should not display thumbnail when task is not active', async () => {
    const addTabToThumbnailTracker = jest.fn()
    const testTabId = 123

    const mockRef = React.createRef<MockContextRef>()

    render(
      <MockContext
        ref={mockRef}
        uiHandler={{ addTabToThumbnailTracker }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Emit contentTaskStarted but task is not active
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Verify no tracker or listener setup when task is not active
    expect(addTabToThumbnailTracker).not.toHaveBeenCalled()

    // Verify no image is rendered
    expect(screen.queryByRole('img')).not.toBeInTheDocument()
  })

  test('should only display thumbnail for matching tabId', async () => {
    const testTabId = 123
    const differentTabId = 456
    const testDataURI =
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='

    const mockRef = React.createRef<MockContextRef>()

    render(
      <MockContext ref={mockRef}>
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Set contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Simulate thumbnail update for different tab
    await act(async () => {
      mockRef.current!.api.emitEvent('thumbnailUpdated', [
        differentTabId,
        testDataURI,
      ])
    })

    // Verify no image appears for different tab
    expect(screen.queryByRole('img')).not.toBeInTheDocument()

    // Now simulate thumbnail update for correct tab
    await act(async () => {
      mockRef.current!.api.emitEvent('thumbnailUpdated', [
        testTabId,
        testDataURI,
      ])
    })

    // Verify image appears for matching tab
    const img = screen.getByRole('img')
    expect(img).toBeInTheDocument()
    expect(img).toHaveAttribute('src', testDataURI)
  })

  test('should cleanup listeners when component unmounts', async () => {
    const addTabToThumbnailTracker = jest.fn()
    const removeTabFromThumbnailTracker = jest.fn()
    const testTabId = 123

    const mockRef = React.createRef<MockContextRef>()

    const { unmount } = render(
      <MockContext
        ref={mockRef}
        uiHandler={{
          addTabToThumbnailTracker,
          removeTabFromThumbnailTracker,
        }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Set contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Verify initial setup
    await waitFor(() => {
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(testTabId)
    })

    // Unmount component
    unmount()

    // Verify cleanup
    await waitFor(() => {
      expect(removeTabFromThumbnailTracker).toHaveBeenCalledWith(testTabId)
    })
  })

  test('should update thumbnail tracker when contentTaskTabId changes', async () => {
    const addTabToThumbnailTracker = jest.fn()
    const removeTabFromThumbnailTracker = jest.fn()
    const initialTabId = 123
    const newTabId = 456

    const mockRef = React.createRef<MockContextRef>()

    render(
      <MockContext
        ref={mockRef}
        uiHandler={{
          addTabToThumbnailTracker,
          removeTabFromThumbnailTracker,
        }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Set initial contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [initialTabId])
    })

    // Verify initial setup
    await waitFor(() => {
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(initialTabId)
    })

    // Change contentTaskTabId by emitting a new contentTaskStarted event
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [newTabId])
    })

    // Verify cleanup of old tab and setup of new tab
    await waitFor(() => {
      expect(removeTabFromThumbnailTracker).toHaveBeenCalledWith(initialTabId)
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(newTabId)
    })
  })

  test('should pass inline search events to AssistantResponse in Progress panel', async () => {
    const searchQuery = 'brave browser'
    const searchResultTitle = 'Brave Browser Result'
    const searchResults = [
      {
        type: 'search_result',
        title: searchResultTitle,
        url: 'https://brave.com',
        description: 'A privacy browser',
        meta_url: {
          favicon: 'https://brave.com/favicon.ico',
          path: '/',
          netloc: 'brave.com',
        },
      },
    ]

    const entriesWithInlineSearch: Mojom.ConversationTurn[] = [
      createConversationTurnWithDefaults({
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getCompletionEvent(`::search[${searchQuery}]{type=web}`),
          {
            ...getEventTemplate(),
            inlineSearchEvent: {
              query: searchQuery,
              resultsJson: JSON.stringify(searchResults),
            },
          },
        ],
      }),
    ]

    render(
      <MockContext>
        <AssistantTask
          assistantEntries={entriesWithInlineSearch}
          isActiveTask={false}
          isGenerating={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // The search result title should be visible because the inline search event
    // was passed to AssistantResponse alongside the completion event.
    await waitFor(() => {
      expect(screen.getByText(searchResultTitle)).toBeInTheDocument()
    })
  })

  test('should cleanup thumbnail tracker when isActiveTask changes to false', async () => {
    const addTabToThumbnailTracker = jest.fn()
    const removeTabFromThumbnailTracker = jest.fn()
    const testTabId = 123

    const mockRef = React.createRef<MockContextRef>()

    const { rerender } = render(
      <MockContext
        ref={mockRef}
        uiHandler={{
          addTabToThumbnailTracker,
          removeTabFromThumbnailTracker,
        }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Set contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Verify initial setup
    await waitFor(() => {
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(testTabId)
    })

    // Change isActiveTask to false
    rerender(
      <MockContext
        ref={mockRef}
        uiHandler={{
          addTabToThumbnailTracker,
          removeTabFromThumbnailTracker,
        }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Verify cleanup
    await waitFor(() => {
      expect(removeTabFromThumbnailTracker).toHaveBeenCalledWith(testTabId)
    })
  })
})

describe('AssistantTask web sources', () => {
  // Simulates the scenario where a server search tool
  // emits sources in an earlier entry and the completion
  // arrives in a later entry.
  // Entry A: ToolUseEvents + WebSourcesEvent
  // Entry B: CompletionEvent
  const entriesWithSourcesInEarlierEntry: Mojom.ConversationTurn[] = [
    createConversationTurnWithDefaults({
      uuid: 'entry-a',
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [
        getToolUseEvent({
          toolName: 'search_web',
          input: '{"query":"test"}',
        }),
        getToolUseEvent({
          toolName: 'get_page',
          input: '{"url":"https://example.com"}',
        }),
        getWebSourcesEvent([
          {
            title: 'Source 1',
            faviconUrl: {
              url: 'https://example.com/fav1.ico',
            },
            url: {
              url: 'https://example.com/page1',
            },
          },
          {
            title: 'Source 2',
            faviconUrl: {
              url: 'https://example.com/fav2.ico',
            },
            url: {
              url: 'https://example.com/page2',
            },
          },
        ]),
        getSearchQueriesEvent(['test query']),
      ],
    }),
    createConversationTurnWithDefaults({
      uuid: 'entry-b',
      characterType: Mojom.CharacterType.ASSISTANT,
      events: [getCompletionEvent('Here is the answer.')],
    }),
  ]

  test('Progress view shows web sources from earlier entries', () => {
    const { container } = render(
      <MockContext>
        <AssistantTask
          assistantEntries={entriesWithSourcesInEarlierEntry}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    expect(
      container.querySelector('[data-test-id="web-sources-event"]'),
    ).toBeInTheDocument()
  })

  test('Progress view shows search summary', () => {
    const { container } = render(
      <MockContext>
        <AssistantTask
          assistantEntries={entriesWithSourcesInEarlierEntry}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    expect(
      container.querySelector('[data-test-id="search-summary"]'),
    ).toBeInTheDocument()
  })

  test('Steps view shows web sources with completion step', () => {
    const { container } = render(
      <MockContext>
        <AssistantTask
          assistantEntries={entriesWithSourcesInEarlierEntry}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Switch to Steps tab via shadowRoot click
    const stepsTab = container.querySelector('leo-tabitem[value="steps"]')
    expect(stepsTab).toBeTruthy()
    act(() => {
      stepsTab?.shadowRoot?.querySelector('button')?.click()
    })

    // Verify sources appear in the Steps view
    expect(
      container.querySelector('[data-test-id="web-sources-event"]'),
    ).toBeInTheDocument()

    // Sources should be in the last step (with the
    // completion), not the first step (tools only).
    const steps = container.querySelectorAll('[class*="taskStep"]')
    expect(steps.length).toBe(2)
    expect(
      steps[0].querySelector('[data-test-id="web-sources-event"]'),
    ).toBeNull()
    expect(
      steps[1].querySelector('[data-test-id="web-sources-event"]'),
    ).toBeInTheDocument()
  })

  test('Steps view shows search summary with completion step', () => {
    const { container } = render(
      <MockContext>
        <AssistantTask
          assistantEntries={entriesWithSourcesInEarlierEntry}
          isActiveTask={false}
          isLeoModel={true}
        />
      </MockContext>,
    )

    // Switch to Steps tab
    const stepsTab = container.querySelector('leo-tabitem[value="steps"]')
    expect(stepsTab).toBeTruthy()
    act(() => {
      stepsTab?.shadowRoot?.querySelector('button')?.click()
    })

    // Search summary should be in the last step
    const steps = container.querySelectorAll('[class*="taskStep"]')
    expect(steps.length).toBe(2)
    expect(steps[0].querySelector('[data-test-id="search-summary"]')).toBeNull()
    expect(
      steps[1].querySelector('[data-test-id="search-summary"]'),
    ).toBeInTheDocument()
  })
})
