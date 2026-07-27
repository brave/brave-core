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
import {
  ProgressBubbleContextProvider,
  useProgressBubbleContext,
} from '../progress_bubble/progress_bubble'
import AssistantTask from './assistant_task'

// Renders AssistantTask inside a ProgressBubbleContextProvider that defaults
// the Steps view to expanded. Used by tests that exercise the Steps panel,
// since switching panels is now driven by the shared context rather than
// internal tabs.
function ExpandSteps() {
  const ctx = useProgressBubbleContext()
  React.useEffect(() => {
    ctx.setIsExpanded(true)
  }, [ctx])
  return null
}

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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
    const addTabToThumbnailTracker = jest.fn()
    const testTabId = 123
    const differentTabId = 456
    const testDataURI =
      'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='

    const mockRef = React.createRef<MockContextRef>()

    render(
      <MockContext
        ref={mockRef}
        uiHandler={{ addTabToThumbnailTracker }}
      >
        <AssistantTask
          assistantEntries={mockAssistantEntries}
          isActiveTask={true}
          allowedLinks={[]}
        />
      </MockContext>,
    )

    // Set contentTaskTabId
    await act(async () => {
      mockRef.current!.api.emitEvent('contentTaskStarted', [testTabId])
    })

    // Wait for subscription to be set up before emitting thumbnail events
    await waitFor(() => {
      expect(addTabToThumbnailTracker).toHaveBeenCalledWith(testTabId)
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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
          allowedLinks={[]}
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
          toolName: Mojom.BRAVE_WEB_SEARCH_TOOL_NAME,
          id: '1',
          argumentsJson: JSON.stringify({ query: ['test'] }),
          output: undefined,
        }),
        getToolUseEvent({
          toolName: 'get_page',
          id: '2',
          argumentsJson: JSON.stringify({ url: 'https://example.com' }),
          output: undefined,
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
          allowedLinks={[]}
        />
      </MockContext>,
    )

    expect(
      container.querySelector('[data-testid="web-sources-event"]'),
    ).toBeInTheDocument()
  })

  test('Steps view shows web sources with completion step', () => {
    const { container } = render(
      <MockContext>
        <ProgressBubbleContextProvider>
          <ExpandSteps />
          <AssistantTask
            assistantEntries={entriesWithSourcesInEarlierEntry}
            isActiveTask={false}
            allowedLinks={[]}
          />
        </ProgressBubbleContextProvider>
      </MockContext>,
    )

    // Verify sources appear in the Steps view
    expect(
      container.querySelector('[data-testid="web-sources-event"]'),
    ).toBeInTheDocument()

    // Sources should be in the last step (with the
    // completion), not the first step (tools only).
    const steps = container.querySelectorAll('[class*="taskStep"]')
    expect(steps.length).toBe(2)
    expect(
      steps[0].querySelector('[data-testid="web-sources-event"]'),
    ).toBeNull()
    expect(
      steps[1].querySelector('[data-testid="web-sources-event"]'),
    ).toBeInTheDocument()
  })

  test('Steps view renders a search summary alongside the search tool use', () => {
    // The search summary is now produced by the per-tool ToolEventSearch
    // component, so it appears in whichever step contains the search tool
    // use event rather than always with the completion.
    const { container } = render(
      <MockContext>
        <ProgressBubbleContextProvider>
          <ExpandSteps />
          <AssistantTask
            assistantEntries={entriesWithSourcesInEarlierEntry}
            isActiveTask={false}
            allowedLinks={[]}
          />
        </ProgressBubbleContextProvider>
      </MockContext>,
    )

    const steps = container.querySelectorAll('[class*="taskStep"]')
    expect(steps.length).toBe(2)
    // The search tool use lives in the first (pre-completion) step.
    expect(
      steps[0].querySelector('[data-testid="search-summary"]'),
    ).toBeInTheDocument()
    expect(steps[1].querySelector('[data-testid="search-summary"]')).toBeNull()
  })
})
