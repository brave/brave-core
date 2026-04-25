// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render, waitFor } from '@testing-library/react'
import MockContext, {
  MockContextRef,
} from '../../mock_untrusted_conversation_context'
import Conversation from '.'

jest.mock('../conversation_entries', () => ({
  __esModule: true,
  default: () => null,
}))

jest.mock('../model_intro', () => ({
  __esModule: true,
  default: () => null,
}))

jest.mock('./useScrollToBottom', () => ({
  useScrollToBottom: () => ({
    scrollToBottomContinuously: jest.fn(),
    hasScrollableContent: false,
  }),
}))

const withSuggestionsState = {
  serviceState: { hasAcceptedAgreement: true },
  conversationEntriesState: { suggestedQuestions: ['Test question'] },
}

describe('Conversation --suggested-questions-height CSS variable', () => {
  let resizeObserverCallbacks: ResizeObserverCallback[]

  beforeEach(() => {
    resizeObserverCallbacks = []
    document.body.style.removeProperty('--suggested-questions-height')
    ;(window as any).ResizeObserver = class MockResizeObserver
      implements ResizeObserver
    {
      constructor(callback: ResizeObserverCallback) {
        resizeObserverCallbacks.push(callback)
      }

      observe() {}
      unobserve() {}
      disconnect() {}
    }
  })

  async function renderWithSuggestionsAndTriggerResize(
    ref?: React.Ref<MockContextRef>,
  ) {
    render(
      <MockContext
        ref={ref}
        initialState={withSuggestionsState}
      >
        <Conversation />
      </MockContext>,
    )
    await act(async () => {
      for (const cb of resizeObserverCallbacks) {
        cb([], {} as ResizeObserver)
      }
    })
  }

  it('sets the CSS variable via ResizeObserver callback when suggestions are visible', async () => {
    await renderWithSuggestionsAndTriggerResize()

    expect(
      document.body.style.getPropertyValue('--suggested-questions-height'),
    ).toBe('calc(0px + var(--leo-spacing-2xl) * 2)')
  })

  it('resets the CSS variable to 0px when suggestions are removed', async () => {
    const mockRef = React.createRef<MockContextRef>()
    await renderWithSuggestionsAndTriggerResize(mockRef)

    await act(async () => {
      mockRef.current!.api.state.update({ suggestedQuestions: [] })
    })

    await waitFor(() => {
      expect(
        document.body.style.getPropertyValue('--suggested-questions-height'),
      ).toBe('0px')
    })
  })

  it('resets the CSS variable to 0px on unmount', () => {
    const { unmount } = render(
      <MockContext initialState={withSuggestionsState}>
        <Conversation />
      </MockContext>,
    )

    unmount()

    expect(
      document.body.style.getPropertyValue('--suggested-questions-height'),
    ).toBe('0px')
  })
})
