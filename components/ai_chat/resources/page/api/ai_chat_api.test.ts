// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import createAIChatApi from './ai_chat_api'
import * as Mojom from '../../common/mojom'
import { makeCloseable } from '$web-common/api'
import { clearAllDataForTesting } from '$web-common/api'
import { createMockService } from './mock_interfaces'

const CG = Mojom.ConversationGenerationState

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

type AIChatApiWrapper = ReturnType<typeof makeApi>

function emit(
  wrapper: AIChatApiWrapper,
  uuid: string,
  state: Mojom.ConversationGenerationState,
) {
  wrapper.serviceObserver.onConversationGenerationStateChanged(uuid, state)
}

function states(wrapper: AIChatApiWrapper) {
  return wrapper.api.conversationGenerationState.current()
}

describe('ai_chat_api conversationGenerationState', () => {
  beforeEach(() => {
    clearAllDataForTesting()
  })

  it('initially empty', () => {
    const wrapper = makeApi()
    expect(states(wrapper).size).toBe(0)
  })

  it('adds kGenerating on event', () => {
    const wrapper = makeApi()
    emit(wrapper, 'uuid-1', CG.kGenerating)
    expect(states(wrapper).get('uuid-1')).toBe(CG.kGenerating)
  })

  it('adds kCompletedNotViewed on event', () => {
    const wrapper = makeApi()
    emit(wrapper, 'uuid-2', CG.kCompletedNotViewed)
    expect(states(wrapper).get('uuid-2')).toBe(CG.kCompletedNotViewed)
  })

  it('transitions kGenerating -> kCompletedNotViewed', () => {
    const wrapper = makeApi()
    emit(wrapper, 'uuid-1', CG.kGenerating)
    emit(wrapper, 'uuid-1', CG.kCompletedNotViewed)
    expect(states(wrapper).get('uuid-1')).toBe(CG.kCompletedNotViewed)
    expect(states(wrapper).size).toBe(1)
  })

  it('kIdle deletes entry', () => {
    const wrapper = makeApi()
    emit(wrapper, 'uuid-1', CG.kGenerating)
    emit(wrapper, 'uuid-1', CG.kIdle)
    expect(states(wrapper).has('uuid-1')).toBe(false)
    expect(states(wrapper).size).toBe(0)
  })

  it('kIdle on unknown id is no-op', () => {
    const wrapper = makeApi()
    emit(wrapper, 'unknown', CG.kIdle)
    expect(states(wrapper).size).toBe(0)
  })

  it('handles multiple conversations independently', () => {
    const wrapper = makeApi()
    emit(wrapper, 'a', CG.kGenerating)
    emit(wrapper, 'b', CG.kCompletedNotViewed)
    emit(wrapper, 'c', CG.kGenerating)
    expect(states(wrapper).size).toBe(3)
    emit(wrapper, 'a', CG.kIdle)
    expect(states(wrapper).size).toBe(2)
    expect(states(wrapper).has('a')).toBe(false)
    expect(states(wrapper).get('b')).toBe(CG.kCompletedNotViewed)
  })

  it('idempotent kGenerating insert', () => {
    const wrapper = makeApi()
    emit(wrapper, 'x', CG.kGenerating)
    const size1 = states(wrapper).size
    emit(wrapper, 'x', CG.kGenerating)
    expect(size1).toBe(1)
    expect(states(wrapper).size).toBe(1)
  })
})
