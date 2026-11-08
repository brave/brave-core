/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, renderHook } from '@testing-library/react'
import { SearchContext } from '../../context/search_context'
import { defaultSearchStore, SearchEngineInfo } from '../../state/search_store'
import { useSearchInputState } from './search_input_state'

const brave: SearchEngineInfo = {
  prepopulateId: 1n,
  host: 'search.brave.com',
  keyword: ':br',
  name: 'Brave',
  faviconUrl: '',
}
const custom: SearchEngineInfo = {
  prepopulateId: 0n,
  host: 'example.com',
  keyword: 'custom',
  name: 'Custom',
  faviconUrl: '',
}

it('keeps the fallback selected when a disabled engine is re-enabled', async () => {
  const store = defaultSearchStore()
  store.update({
    initialized: true,
    searchEngines: [brave, custom],
    enabledSearchEngines: new Set([brave.host, 'keyword:custom']),
    lastUsedSearchEngine: 'keyword:custom',
  })
  const { result } = renderHook(() => useSearchInputState('test'), {
    wrapper: ({ children }) =>
      React.createElement(SearchContext.Provider, { value: store }, children),
  })
  expect(result.current.searchEngine).toEqual(custom)

  await act(async () => {
    store.update({ enabledSearchEngines: new Set([brave.host]) })
  })
  expect(result.current.searchEngine).toEqual(brave)

  await act(async () => {
    store.update({
      enabledSearchEngines: new Set([brave.host, 'keyword:custom']),
    })
  })
  expect(result.current.searchEngine).toEqual(brave)
})

it('restores the saved custom engine when preferences arrive after mounting', async () => {
  const store = defaultSearchStore()
  const { result } = renderHook(() => useSearchInputState('test'), {
    wrapper: ({ children }) =>
      React.createElement(SearchContext.Provider, { value: store }, children),
  })
  await act(async () => {
    store.update({
      searchEngines: [brave, custom],
      enabledSearchEngines: new Set([brave.host, 'keyword:custom']),
    })
  })
  await act(async () => {
    store.update({
      initialized: true,
      lastUsedSearchEngine: 'keyword:custom',
    })
  })
  expect(result.current.searchEngine).toEqual(custom)
})
