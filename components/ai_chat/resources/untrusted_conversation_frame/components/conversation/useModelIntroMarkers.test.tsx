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

  it('replaces the top marker when changing models again before sending', async () => {
    const { result, mockRef } = renderModelIntroMarkersHook({
      conversationEntriesState: {
        currentModelKey: 'default-model',
        defaultModelKey: 'default-model',
      },
    })

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
      afterPairIndex: null,
    })

    await act(async () => {
      mockRef.current!.api.state.update({
        currentModelKey: 'second-model',
        defaultModelKey: 'default-model',
      })
    })

    // Wait for the modelKey itself — length stays 1 across the replace, so
    // asserting only on length can pass before the pending marker is updated.
    await waitFor(() => {
      expect(result.current).toHaveLength(1)
      expect(result.current[0]).toMatchObject({
        modelKey: 'second-model',
        afterPairIndex: null,
      })
    })
  })

  it('replaces the pending inline marker when changing models before the next prompt', async () => {
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

    // Wait for the modelKey itself — length stays 1 across the replace, so
    // asserting only on length can pass before the pending marker is updated.
    await waitFor(() => {
      expect(result.current).toHaveLength(1)
      expect(result.current[0]).toMatchObject({
        modelKey: 'second-model',
        afterPairIndex: 0,
      })
    })
  })

  it('keeps earlier markers and adds a new one after another prompt is sent', async () => {
    const firstTurn = createConversationTurnWithDefaults({
      uuid: 'turn-1',
      characterType: Mojom.CharacterType.ASSISTANT,
      text: 'First reply',
    })
    const { result, mockRef } = renderModelIntroMarkersHook(
      {
        conversationEntriesState: {
          currentModelKey: 'default-model',
          defaultModelKey: 'default-model',
        },
        conversationHistory: [firstTurn],
      },
      [firstTurn],
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

    const secondTurn = createConversationTurnWithDefaults({
      uuid: 'turn-2',
      characterType: Mojom.CharacterType.HUMAN,
      text: 'Follow-up',
    })
    const thirdTurn = createConversationTurnWithDefaults({
      uuid: 'turn-3',
      characterType: Mojom.CharacterType.ASSISTANT,
      text: 'Second reply',
    })

    await act(async () => {
      mockRef.current!.api.getConversationHistory.update([
        firstTurn,
        secondTurn,
        thirdTurn,
      ])
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
      afterPairIndex: 1,
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
