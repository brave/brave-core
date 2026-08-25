// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import createAIChatApi from './ai_chat_api'
import * as Mojom from '../../common/mojom'
import { ConversationGenerationState } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { makeCloseable } from '$web-common/api'
import { clearAllDataForTesting } from '$web-common/api'
import { createMockService } from './mock_interfaces'

const CG = ConversationGenerationState

function makeApi() {
  const mockService = createMockService()
  const mockUI = makeCloseable({} as unknown as Mojom.AIChatUIHandlerInterface)
  const mockBookmarks = makeCloseable(
    {} as unknown as Mojom.BookmarksPageHandlerInterface,
  )
  const mockHistory = makeCloseable(
    {} as unknown as Mojom.HistoryUIHandlerInterface,
  )
  const mockMetrics = makeCloseable({} as unknown as Mojom.MetricsInterface)
  const wrapper = createAIChatApi(
    mockService,
    mockUI,
    mockBookmarks,
    mockHistory,
    mockMetrics,
  )
  return wrapper
}

describe('ai_chat_api conversationGenerationState', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('initially empty', () => {
    const wrapper = makeApi()
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(0)
  })

  it('adds kGenerating on event', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-1',
      CG.kGenerating,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().get('uuid-1'),
    ).toBe(CG.kGenerating)
  })

  it('adds kCompletedNotViewed on event', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-2',
      CG.kCompletedNotViewed,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().get('uuid-2'),
    ).toBe(CG.kCompletedNotViewed)
  })

  it('transitions kGenerating -> kCompletedNotViewed', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-1',
      CG.kGenerating,
    )
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-1',
      CG.kCompletedNotViewed,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().get('uuid-1'),
    ).toBe(CG.kCompletedNotViewed)
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(1)
  })

  it('kIdle deletes entry', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-1',
      CG.kGenerating,
    )
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'uuid-1',
      CG.kIdle,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().has('uuid-1'),
    ).toBe(false)
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(0)
  })

  it('kIdle on unknown id is no-op', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'unknown',
      CG.kIdle,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(0)
  })

  it('handles multiple conversations independently', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'a',
      CG.kGenerating,
    )
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'b',
      CG.kCompletedNotViewed,
    )
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'c',
      CG.kGenerating,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(3)
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'a',
      CG.kIdle,
    )
    expect(
      (wrapper.api as any).conversationGenerationState.current().size,
    ).toBe(2)
    expect(
      (wrapper.api as any).conversationGenerationState.current().has('a'),
    ).toBe(false)
    expect(
      (wrapper.api as any).conversationGenerationState.current().get('b'),
    ).toBe(CG.kCompletedNotViewed)
  })

  it('idempotent kGenerating insert', () => {
    const wrapper = makeApi()
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'x',
      CG.kGenerating,
    )
    const size1 = (wrapper.api as any).conversationGenerationState.current()
      .size
    ;(wrapper.serviceObserver as any).onConversationGenerationStateChanged(
      'x',
      CG.kGenerating,
    )
    const size2 = (wrapper.api as any).conversationGenerationState.current()
      .size
    expect(size1).toBe(1)
    expect(size2).toBe(1)
  })
})
