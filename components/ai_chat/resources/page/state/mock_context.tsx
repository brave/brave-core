// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../common/mojom'
import createAIChatApi from '../api/ai_chat_api'
import createConversationApi from '../api/conversation_api'
import {
  createMockConversationHandler,
  createMockService,
  createMockUIHandler,
  createMockBookmarksService,
  createMockHistoryService,
  createMockMetrics,
  defaultConversationState,
  defaultServiceState,
} from '../api/mock_interfaces'
import { AIChatContext, AIChatProvider } from './ai_chat_context'
import {
  ConversationContext,
  ConversationContextProps,
  ConversationProvider,
} from './conversation_context'

export interface MockContextProps {
  children: React.ReactNode

  // Direct Mojo interface overrides
  service?: Partial<Mojom.ServiceInterface>
  uiHandler?: Partial<Mojom.AIChatUIHandlerInterface>
  bookmarksService?: Partial<Mojom.BookmarksPageHandlerInterface>
  historyService?: Partial<Mojom.HistoryUIHandlerInterface>
  metrics?: Partial<Mojom.MetricsInterface>
  conversationHandler?: Partial<Mojom.ConversationHandlerInterface>

  // Initial state for state-type endpoints
  initialState?: {
    tabs?: Mojom.TabData[]
    isStandalone?: boolean
    serviceState?: Partial<Mojom.ServiceState>
    conversationState?: Partial<Mojom.ConversationState>
  }

  // Context-level overrides (for things computed in the provider hooks)
  aiChatOverrides?: Partial<AIChatContext>
  conversationOverrides?: Partial<ConversationContext>

  // Context-level props
  conversationProps?: Partial<ConversationContextProps>

  deps?: React.DependencyList
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
 * // Get access to the api
 * it('handled an event', () => {\
 *   const wrapper = ({ children }: { children: React.ReactNode }) => (
       <MockContext>{children}</MockContext>
 *   )
 *
 *   const onDragStart = jest.fn()
 *
 *   act(() => {
 *     return renderHook(() => {
 *       const aiChat = useAIChat()
 *       aiChat.useOnDragStart(onDragStart)
 *       useAIChat().api.emitEvent('dragStart', [])
 *     }, {
 *       wrapper,
 *     })
 *   })
 *
 *   expect(onDragStart).toHaveBeenCalled()
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
    initialState = {},
    aiChatOverrides,
    conversationHandler,
    conversationOverrides,
    conversationProps = {},
    deps = [],
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

    return api
  })

  const [conversationApi] = React.useState(() => {
    const mockHandler = createMockConversationHandler(
      conversationHandler,
      initialState.conversationState ?? {},
    )

    const conversation = createConversationApi(mockHandler)

    return conversation
  })

  React.useLayoutEffect(() => {
    if (deps.length) {
      aiChatApi.api.invalidateAll()
      conversationApi.api.invalidateAll()
      // Ensure that this effect is followed by the state.update effect so
      // we re-populate with intended data.
    }
  }, [deps])

  React.useLayoutEffect(() => {
    // Update anything that doesn't have a query and is provided via events
    aiChatApi.api.state.update({
      ...defaultServiceState,
      ...initialState.serviceState,
    })

    aiChatApi.api.isStandalone.update(initialState.isStandalone ?? false)

    if (initialState.tabs) {
      aiChatApi.api.tabs.update(initialState.tabs)
    }

    if (initialState.conversationState) {
      conversationApi.api.getState.update({
        ...defaultConversationState,
        ...initialState.conversationState,
      })
    }
  }, [
    deps,
    initialState.serviceState,
    initialState.isStandalone,
    initialState.tabs,
    initialState.conversationState,
  ])

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
      <ConversationProvider
        api={conversationApi.api}
        selectedConversationId={
          initialState.conversationState?.conversationUuid
          ?? defaultConversationState.conversationUuid
        }
        updateSelectedConversationId={() => {}}
        createNewConversation={() => {}}
        isTabAssociated={false}
        {...conversationProps}
        overrides={conversationOverrides}
      >
        {children}
      </ConversationProvider>
    </AIChatProvider>
  )
}
