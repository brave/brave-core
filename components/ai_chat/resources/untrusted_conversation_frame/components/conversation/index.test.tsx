// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, render, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import MockContext, {
  MockContextRef,
} from '../../mock_untrusted_conversation_context'
import { createConversationTurnWithDefaults } from '../../../common/test_data_utils'
import Conversation from '.'

jest.mock('../conversation_entries', () => ({
  __esModule: true,
  default: () => <div data-testid='conversation-entries' />,
}))

jest.mock('../model_intro', () => ({
  __esModule: true,
  default: ({ modelKey }: { modelKey: string }) => (
    <div
      data-testid='model-intro'
      data-model-key={modelKey}
    />
  ),
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

describe('Conversation ModelIntro placement', () => {
  it('renders top markers before conversation entries', async () => {
    const mockRef = React.createRef<MockContextRef>()
    const { getByTestId } = render(
      <MockContext
        ref={mockRef}
        initialState={{
          conversationEntriesState: {
            currentModelKey: 'default-model',
            defaultModelKey: 'default-model',
          },
        }}
      >
        <Conversation />
      </MockContext>,
    )

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'test-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(getByTestId('model-intro')).toBeInTheDocument()
    })

    const modelIntro = getByTestId('model-intro')
    const conversationEntries = getByTestId('conversation-entries')
    expect(modelIntro).toHaveAttribute('data-model-key', 'test-model')
    expect(
      modelIntro.compareDocumentPosition(conversationEntries)
        & Node.DOCUMENT_POSITION_FOLLOWING,
    ).toBeTruthy()
  })

  it('keeps top markers after the conversation starts', async () => {
    const mockRef = React.createRef<MockContextRef>()
    const { getAllByTestId } = render(
      <MockContext
        ref={mockRef}
        initialState={{
          conversationEntriesState: {
            currentModelKey: 'default-model',
            defaultModelKey: 'default-model',
          },
        }}
      >
        <Conversation />
      </MockContext>,
    )

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'test-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(getAllByTestId('model-intro')).toHaveLength(1)
    })

    await act(async () => {
      mockRef.current!.api.getConversationHistory.update([
        createConversationTurnWithDefaults({
          uuid: 'turn-1',
          characterType: Mojom.CharacterType.HUMAN,
          text: 'Hello',
        }),
      ])
    })

    await waitFor(() => {
      expect(getAllByTestId('model-intro')).toHaveLength(1)
    })
    expect(getAllByTestId('model-intro')[0]).toHaveAttribute(
      'data-model-key',
      'test-model',
    )
  })
})

describe('Conversation --notices-height CSS variable', () => {
  let resizeObserverCallbacks: ResizeObserverCallback[]

  let rafCallbacks: FrameRequestCallback[]

  beforeEach(() => {
    resizeObserverCallbacks = []
    rafCallbacks = []
    document.body.style.removeProperty('--notices-height')
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
    jest
      .spyOn(window, 'requestAnimationFrame')
      .mockImplementation((cb: FrameRequestCallback) => {
        rafCallbacks.push(cb)
        return 0
      })
  })

  afterEach(() => {
    jest.restoreAllMocks()
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
      for (const cb of rafCallbacks) {
        // eslint-disable-next-line n/no-callback-literal
        cb(0)
      }
    })
  }

  it('sets the CSS variable via ResizeObserver callback when suggestions are visible', async () => {
    jest.spyOn(HTMLElement.prototype, 'getBoundingClientRect').mockReturnValue({
      height: 80,
      width: 0,
      top: 0,
      left: 0,
      bottom: 0,
      right: 0,
      x: 0,
      y: 0,
      toJSON: () => ({}),
    } as DOMRect)

    await renderWithSuggestionsAndTriggerResize()

    expect(document.body.style.getPropertyValue('--notices-height')).toBe(
      'calc(80px + var(--leo-spacing-2xl) * 2)',
    )
  })

  it('resets the CSS variable to 0px when suggestions are removed', async () => {
    const mockRef = React.createRef<MockContextRef>()
    await renderWithSuggestionsAndTriggerResize(mockRef)

    await act(async () => {
      mockRef.current!.api.state.update({ suggestedQuestions: [] })
    })

    await waitFor(() => {
      expect(document.body.style.getPropertyValue('--notices-height')).toBe(
        '0px',
      )
    })
  })

  it('resets the CSS variable to 0px on unmount', () => {
    const { unmount } = render(
      <MockContext initialState={withSuggestionsState}>
        <Conversation />
      </MockContext>,
    )

    unmount()

    expect(document.body.style.getPropertyValue('--notices-height')).toBe('0px')
  })
})
