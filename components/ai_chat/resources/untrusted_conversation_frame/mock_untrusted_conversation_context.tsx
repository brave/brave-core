// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../common/mojom'
import createUntrustedConversationApi, {
  UntrustedConversationAPI,
} from './api/untrusted_conversation_api'
import {
  createMockUntrustedConversationHandler,
  createMockUntrustedUIHandler,
  createMockParentUIFrame,
  defaultConversationEntriesState,
} from './api/mock_interfaces'
import {
  UntrustedConversationContext,
  UntrustedConversationContextProvider,
} from './untrusted_conversation_context'

export interface MockContextProps {
  children: React.ReactNode

  // Direct Mojo interface overrides
  conversationHandler?: Partial<Mojom.UntrustedConversationHandlerInterface>
  uiHandler?: Partial<Mojom.UntrustedUIHandlerInterface>
  parentUIFrame?: Partial<Mojom.ParentUIFrameInterface>

  // Initial state for data endpoints
  initialState?: {
    conversationEntriesState?: Partial<Mojom.ConversationEntriesState>
    conversationHistory?: Mojom.ConversationTurn[]
  }

  // Context-level overrides (for things computed in the provider hook)
  overrides?: Partial<UntrustedConversationContext>

  deps?: React.DependencyList
}

/**
 * Exposes the underlying API instance for tests that need to emit events.
 */
export interface MockContextRef {
  api: UntrustedConversationAPI
}

/**
 * A test wrapper component that provides a mock Untrusted Conversation context.
 *
 * Use this to wrap components under test that need access to
 * useUntrustedConversationContext().
 *
 * @example
 * ```tsx
 * // Basic usage
 * render(<MockContext><MyComponent /></MockContext>)
 *
 * // With initial conversation history
 * render(
 *   <MockContext initialState={{ conversationHistory: myHistory }}>
 *     <MyComponent />
 *   </MockContext>
 * )
 *
 * // With interface overrides for testing actions
 * render(
 *   <MockContext conversationHandler={{ respondToToolUseRequest: jest.fn() }}>
 *     <MyComponent />
 *   </MockContext>
 * )
 *
 * // With ref for emitting events in tests
 * const mockRef = React.createRef<MockContextRef>()
 * render(<MockContext ref={mockRef}><MyComponent /></MockContext>)
 * await act(() => mockRef.current!.api.emitEvent('thumbnailUpdated', [tabId, dataUri]))
 * ```
 */
const MockContext = React.forwardRef<MockContextRef, MockContextProps>(
  function MockContext(props, ref) {
    const {
      children,
      conversationHandler,
      uiHandler,
      parentUIFrame,
      initialState = {},
      overrides,
      deps = [],
    } = props

    const [untrustedApi] = React.useState(() => {
      const mockConversationHandler =
        createMockUntrustedConversationHandler(conversationHandler)
      const mockUIHandler = createMockUntrustedUIHandler(uiHandler)
      const mockParentUIFrame = createMockParentUIFrame(parentUIFrame)

      return createUntrustedConversationApi(
        mockConversationHandler,
        mockUIHandler,
        mockParentUIFrame,
      )
    })

    React.useImperativeHandle(ref, () => ({ api: untrustedApi.api }))

    React.useLayoutEffect(() => {
      if (deps.length) {
        untrustedApi.api.invalidateAll()
      }
    }, [deps, untrustedApi.api])

    React.useLayoutEffect(() => {
      if (initialState.conversationEntriesState) {
        untrustedApi.api.state.update({
          ...defaultConversationEntriesState,
          ...initialState.conversationEntriesState,
        })
      }

      if (initialState.conversationHistory) {
        untrustedApi.api.getConversationHistory.update(
          initialState.conversationHistory,
        )
      }
    }, [
      deps,
      initialState.conversationEntriesState,
      initialState.conversationHistory,
      untrustedApi.api,
    ])

    return (
      <UntrustedConversationContextProvider
        api={untrustedApi.api}
        overrides={overrides}
      >
        {children}
      </UntrustedConversationContextProvider>
    )
  },
)

export default MockContext
