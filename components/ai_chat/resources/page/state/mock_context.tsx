// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Test utilities for AI Chat components.
 *
 * Provides a MockContext component that wraps components with properly
 * configured mock APIs, making it easy to test components that use
 * useAIChat() and useConversation() hooks.
 *
 * This component accepts direct Mojo interface overrides, matching the pattern
 * used in mock_interfaces.ts and components_panel.tsx.
 */

import * as React from 'react'
import * as Mojom from '../../common/mojom'
import createAIChatApi from '../api'
import createConversationApi from '../api/conversation_api'
import {
  createMockConversationHandler,
  createMockService,
  createMockUIHandler,
  createMockBookmarksService,
  createMockHistoryService,
  createMockMetrics,
} from '../api/mock_interfaces'
import { AIChatContext, AIChatProvider } from './ai_chat_context'
import {
  ConversationContext,
  ConversationProvider,
} from './conversation_context'
import { ActiveChatContext, SelectedChatDetails } from './active_chat_context'

export interface MockContextProps {
  children: React.ReactNode

  // Direct Mojo interface overrides (all optional, defaults from mock_interfaces.ts)
  service?: Partial<Mojom.ServiceInterface>
  uiHandler?: Partial<Mojom.AIChatUIHandlerInterface>
  bookmarksService?: Partial<Mojom.BookmarksPageHandlerInterface>
  historyService?: Partial<Mojom.HistoryUIHandlerInterface>
  metrics?: Partial<Mojom.MetricsInterface>
  conversationHandler?: Partial<Mojom.ConversationHandlerInterface>

  // Initial state for state-type endpoints (tabs, isStandalone, etc.)
  initialState?: {
    tabs?: Mojom.TabData[]
    isStandalone?: boolean
    serviceState?: Partial<Mojom.ServiceState>
    conversationState?: Partial<Mojom.ConversationState>
  }

  // Context-level overrides (for things computed in the provider hooks)
  aiChatOverrides?: Partial<AIChatContext>
  conversationOverrides?: Partial<ConversationContext>
}

/**
 * A test wrapper component that provides mock AI Chat and Conversation contexts.
 *
 * Use this to wrap components under test that need access to useAIChat()
 * or useConversation() hooks.
 *
 * @example
 * ```tsx
 * import { MockContext } from '../../state/mock_context'
 *
 * // Test with tabs
 * it('renders tabs', () => {
 *   const { getByText } = render(
 *     <MockContext initialState={{ tabs: SAMPLE_TABS }}>
 *       <MyComponent />
 *     </MockContext>
 *   )
 *   expect(getByText('Test')).toBeInTheDocument()
 * })
 *
 * // Test with bookmarks
 * it('renders bookmarks', () => {
 *   const { getByText } = render(
 *     <MockContext
 *       bookmarksService={{
 *         getBookmarks: () => Promise.resolve({ bookmarks: SAMPLE_BOOKMARKS })
 *       }}
 *     >
 *       <MyComponent />
 *     </MockContext>
 *   )
 * })
 *
 * // Test with callback
 * it('calls associateTab', () => {
 *   const associateTab = jest.fn()
 *   render(
 *     <MockContext uiHandler={{ associateTab }}>
 *       <MyComponent />
 *     </MockContext>
 *   )
 * })
 * ```
 */
export function MockContext(props: MockContextProps) {
  const {
    children,
    service,
    uiHandler,
    bookmarksService,
    historyService,
    metrics,
    conversationHandler,
    initialState = {},
    aiChatOverrides,
    conversationOverrides,
  } = props

  // Use ref to hold current props for mock functions that need reactive data
  const propsRef = React.useRef(props)
  propsRef.current = props

  // Create mock APIs with the provided overrides
  const [aiChatApi] = React.useState(() => {
    const mockService = createMockService(service)
    const mockUIHandler = createMockUIHandler(uiHandler)
    const mockBookmarksService = createMockBookmarksService(bookmarksService)
    const mockHistoryService = createMockHistoryService(historyService)
    const mockMetrics = createMockMetrics(metrics)

    const api = createAIChatApi(
      mockService,
      mockUIHandler,
      mockBookmarksService,
      mockHistoryService,
      mockMetrics,
    )

    // Set initial service state
    api.api.state.update({
      hasAcceptedAgreement: true,
      isStoragePrefEnabled: true,
      isStorageNoticeDismissed: true,
      canShowPremiumPrompt: false,
      ...initialState.serviceState,
    })

    // Set initial standalone state
    api.api.isStandalone.update(initialState.isStandalone ?? false)

    // Set initial tabs
    if (initialState.tabs) {
      api.api.tabs.update(initialState.tabs)
    }

    return api
  })

  const [conversationApi] = React.useState(() => {
    const conversationState: Mojom.ConversationState = {
      conversationUuid: 'test-conversation',
      isRequestInProgress: false,
      currentModelKey: 'test-model',
      defaultModelKey: 'test-model',
      allModels: [],
      suggestedQuestions: [],
      suggestionStatus: Mojom.SuggestionGenerationStatus.None,
      associatedContent: [],
      error: Mojom.APIError.None,
      temporary: false,
      toolUseTaskState: Mojom.TaskState.kNone,
      ...initialState.conversationState,
    }
    const mockHandler = createMockConversationHandler({
      getState: () => Promise.resolve({ conversationState }),
      getConversationHistory: () =>
        Promise.resolve({ conversationHistory: [] }),
      ...conversationHandler,
    })

    const conversation = createConversationApi(mockHandler)

    // Update anything we can synchronously so some tests don't have to wait for
    // the query.
    conversation.api.getState.update((old) => ({
      ...old,
      ...conversationState,
    }))

    return conversation
  })

  const activeChatContext: SelectedChatDetails = React.useMemo(
    () => ({
      api: conversationApi.api,
      selectedConversationId: 'test-conversation',
      updateSelectedConversationId: () => {},
      createNewConversation: () => {},
      isTabAssociated: false,
    }),
    [conversationApi.api],
  )

  return (
    <AIChatProvider
      api={aiChatApi.api}
      conversationEntriesComponent={() => <div />}
      overrides={{
        initialized: true,
        isMobile: false,
        isHistoryFeatureEnabled: true,
        ...aiChatOverrides,
      }}
    >
      <ActiveChatContext.Provider value={activeChatContext}>
        <ConversationProvider
          api={conversationApi.api}
          selectedConversationId='test-conversation'
          updateSelectedConversationId={() => {}}
          createNewConversation={() => {}}
          isTabAssociated={false}
          overrides={conversationOverrides}
        >
          {children}
        </ConversationProvider>
      </ActiveChatContext.Provider>
    </AIChatProvider>
  )
}
