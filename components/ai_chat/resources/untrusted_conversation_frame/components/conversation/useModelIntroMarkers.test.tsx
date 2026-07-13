// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, renderHook, waitFor } from '@testing-library/react'
import * as Mojom from '../../../common/mojom'
import { createConversationTurnWithDefaults } from '../../../common/test_data_utils'
import MockContext, {
  MockContextRef,
} from '../../mock_untrusted_conversation_context'
import { useModelIntroMarkers } from './useModelIntroMarkers'

describe('useModelIntroMarkers', () => {
  function renderModelIntroMarkersHook(
    initialState?: React.ComponentProps<typeof MockContext>['initialState'],
    conversationHistory?: Mojom.ConversationTurn[],
  ) {
    const mockRef = React.createRef<MockContextRef>()
    const history = conversationHistory ?? initialState?.conversationHistory
    const wrapper = ({ children }: { children: React.ReactNode }) => (
      <MockContext
        ref={mockRef}
        conversationHandler={
          history
            ? {
                getConversationHistory() {
                  return Promise.resolve({ conversationHistory: history })
                },
              }
            : undefined
        }
        initialState={initialState}
      >
        {children}
      </MockContext>
    )

    const result = renderHook(() => useModelIntroMarkers(), { wrapper })
    return { ...result, mockRef }
  }

  it('creates a top marker when switching to a non-default model before messages', async () => {
    const { result, mockRef } = renderModelIntroMarkersHook({
      conversationEntriesState: {
        currentModelKey: 'default-model',
        defaultModelKey: 'default-model',
      },
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'test-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(result.current).toHaveLength(1)
    })
    expect(result.current[0]).toMatchObject({
      modelKey: 'test-model',
      afterPairIndex: null,
    })
  })

  it('creates inline markers for each non-default model change during a conversation', async () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn-1',
        characterType: Mojom.CharacterType.ASSISTANT,
        text: 'Latest reply',
      }),
    ]
    const { result, mockRef } = renderModelIntroMarkersHook(
      {
        conversationEntriesState: {
          currentModelKey: 'default-model',
          defaultModelKey: 'default-model',
        },
        conversationHistory,
      },
      conversationHistory,
    )

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'first-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(result.current).toHaveLength(1)
    })
    expect(result.current[0]).toMatchObject({
      modelKey: 'first-model',
      afterPairIndex: 0,
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'second-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(result.current).toHaveLength(2)
    })
    expect(result.current[0]).toMatchObject({
      modelKey: 'first-model',
      afterPairIndex: 0,
    })
    expect(result.current[1]).toMatchObject({
      modelKey: 'second-model',
      afterPairIndex: 0,
    })
  })

  it('does not create a marker when switching back to the default model', async () => {
    const { result, mockRef } = renderModelIntroMarkersHook({
      conversationEntriesState: {
        currentModelKey: 'default-model',
        defaultModelKey: 'default-model',
      },
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'test-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(result.current).toHaveLength(1)
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'default-model',
        defaultModelKey: 'default-model',
      })
    })

    expect(result.current).toHaveLength(1)
    expect(result.current[0].modelKey).toBe('test-model')
  })

  it('keeps top markers after the conversation starts', async () => {
    const { result, mockRef } = renderModelIntroMarkersHook({
      conversationEntriesState: {
        currentModelKey: 'default-model',
        defaultModelKey: 'default-model',
      },
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'test-model',
        defaultModelKey: 'default-model',
      })
    })

    await waitFor(() => {
      expect(result.current).toHaveLength(1)
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
      expect(result.current).toHaveLength(1)
    })
    expect(result.current[0]).toMatchObject({
      modelKey: 'test-model',
      afterPairIndex: null,
    })
  })
})
