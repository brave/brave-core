/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, render, cleanup } from '@testing-library/react'
import { EngineContextProvider, useEngineContext } from './EngineContext'

let mockLastUsed = ''
const mockSetLast = jest.fn()
jest.mock('../../hooks/usePref', () => ({
  useNewTabPref: () => [mockLastUsed, mockSetLast],
}))
jest.mock('./config', () => ({
  ENABLED_SEARCH_ENGINES_KEY: 'search-engines',
  LAST_SEARCH_ENGINE_KEY: 'last-search-engine',
  defaultSearchHost: 'search.brave.com',
}))

describe('legacy NTP engine preferences', () => {
  let context: ReturnType<typeof useEngineContext>
  function Probe() {
    context = useEngineContext()
    return null
  }
  function renderContext() {
    act(() => {
      render(
        React.createElement(
          EngineContextProvider,
          null,
          React.createElement(Probe),
        ),
      )
    })
  }
  beforeEach(() => {
    localStorage.clear()
    mockSetLast.mockClear()
    mockLastUsed = 'keyword:custom'
  })
  afterEach(cleanup)

  it('restores a saved custom engine ID', () => {
    localStorage.setItem(
      'search-engines',
      JSON.stringify({ 'keyword:custom': true }),
    )
    renderContext()
    expect(context.lastSearchEngine).toBe('keyword:custom')
    expect(context.engineConfig['keyword:custom']).toBe(true)
  })

  it('writes custom IDs when selecting an engine', () => {
    renderContext()
    act(() =>
      context.setLastSearchEngine({
        prepopulateId: 0n,
        name: 'Custom',
        keyword: 'custom',
        host: 'example.com',
        faviconUrl: { url: '' },
      }),
    )
    expect(mockSetLast).toHaveBeenCalledWith('keyword:custom')
  })

  it('preserves empty Google keys when a custom selection is disabled', () => {
    localStorage.setItem(
      'search-engines',
      JSON.stringify({
        'keyword:custom': false,
        '': true,
      }),
    )
    renderContext()
    expect(context.lastSearchEngine).toBe('')
  })
})
