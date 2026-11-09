/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createSearchStore } from './browser_search_store'
import { SearchBoxProxy } from './search_box_proxy'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { SearchEngineInfo } from './search_store'
import { getSearchEngineId } from '$web-common/search_engine_id'

jest.mock('./new_tab_page_proxy')
jest.mock('./search_box_proxy')
jest.mock('$web-common/loadTimeData', () => ({
  loadTimeData: {
    getBoolean: () => true,
    getString: () => 'search.brave.com',
  },
}))

const brave: SearchEngineInfo = {
  prepopulateId: 1n,
  host: 'search.brave.com',
  keyword: ':br',
  name: 'Brave',
  faviconUrl: '',
}
const web: SearchEngineInfo = {
  prepopulateId: 0n,
  host: 'example.com',
  keyword: 'custom-web',
  name: 'Web',
  faviconUrl: '',
}
const images = { ...web, keyword: 'custom-images', name: 'Images' }

function createPageProxy() {
  return {
    handler: {
      getAvailableSearchEngines: jest.fn().mockResolvedValue({
        searchEngines: [brave, web, images],
      }),
      getShowSearchBox: jest.fn().mockResolvedValue({ showSearchBox: true }),
      getShowChatInput: jest.fn().mockResolvedValue({ showChatInput: false }),
      getSearchSuggestionsEnabled: jest
        .fn()
        .mockResolvedValue({ enabled: false }),
      getSearchSuggestionsPromptDismissed: jest.fn().mockResolvedValue({
        dismissed: true,
      }),
      getLastUsedSearchEngine: jest
        .fn()
        .mockResolvedValue({ engine: brave.host }),
      setLastUsedSearchEngine: jest.fn(),
      openSearch: jest.fn(),
      reportSearchEngineUsage: jest.fn(),
    },
    addListeners: jest.fn(),
  }
}

describe('custom NTP search engines', () => {
  let page: ReturnType<typeof createPageProxy>
  let search: {
    handler: { queryAutocomplete: jest.Mock }
    addListeners: jest.Mock
  }

  async function loadStore() {
    const store = createSearchStore()
    await jest.runOnlyPendingTimersAsync()
    expect(store.getState().initialized).toBe(true)
    return store
  }

  beforeEach(() => {
    jest.useFakeTimers()
    localStorage.clear()
    page = createPageProxy()
    search = {
      handler: { queryAutocomplete: jest.fn() },
      addListeners: jest.fn(),
    }
    ;(NewTabPageProxy.getInstance as jest.Mock).mockReturnValue(page)
    ;(SearchBoxProxy.getInstance as jest.Mock).mockReturnValue(search)
  })

  afterEach(() => jest.useRealTimers())

  it('persists separate selections for engines on the same host', async () => {
    const store = await loadStore()
    const { actions } = store.getState()
    actions.setSearchEngineEnabled(getSearchEngineId(web), true)
    actions.setSearchEngineEnabled(getSearchEngineId(images), true)
    actions.setSearchEngineEnabled(brave.host, false)

    const restored = await loadStore()
    expect([...restored.getState().enabledSearchEngines]).toEqual([
      getSearchEngineId(web),
      getSearchEngineId(images),
    ])
    restored
      .getState()
      .actions.setSearchEngineEnabled(getSearchEngineId(web), false)
    expect([...restored.getState().enabledSearchEngines]).toEqual([
      getSearchEngineId(images),
    ])
  })

  it('uses the selected custom engine for queries, searches and preferences', async () => {
    const store = await loadStore()
    const { actions } = store.getState()
    const id = getSearchEngineId(images)
    actions.queryAutocomplete('trees', id)
    expect(search.handler.queryAutocomplete.mock.calls[0][2]).toBe(
      'custom-images trees',
    )
    actions.setLastUsedSearchEngine(id)
    expect(page.handler.setLastUsedSearchEngine).toHaveBeenCalledWith(id)
    const event = {
      button: 0,
      altKey: false,
      ctrlKey: false,
      metaKey: false,
      shiftKey: false,
    }
    actions.openSearch('trees', id, event)
    expect(page.handler.openSearch).toHaveBeenCalledWith('trees', id, event)
    actions.reportSearchEngineUsage(id)
    expect(page.handler.reportSearchEngineUsage).toHaveBeenCalledWith(0n)
  })

  it('falls back when the saved custom engine was deleted', async () => {
    localStorage.setItem(
      'search-engines',
      JSON.stringify({
        [getSearchEngineId(images)]: true,
      }),
    )
    page.handler.getAvailableSearchEngines.mockResolvedValue({
      searchEngines: [brave],
    })
    page.handler.getLastUsedSearchEngine.mockResolvedValue({
      engine: getSearchEngineId(images),
    })
    const store = await loadStore()
    expect([...store.getState().enabledSearchEngines]).toEqual([brave.host])
    expect(store.getState().lastUsedSearchEngine).toBe(brave.host)
    expect(page.handler.setLastUsedSearchEngine).toHaveBeenCalledWith(
      brave.host,
    )
  })

  it('preserves built-in Google preference migration', async () => {
    localStorage.setItem(
      'search-engines',
      JSON.stringify({ 'google.com': true }),
    )
    page.handler.getAvailableSearchEngines.mockResolvedValue({
      searchEngines: [{ ...brave, host: 'www.google.com' }],
    })
    const store = await loadStore()
    expect([...store.getState().enabledSearchEngines]).toEqual([
      'www.google.com',
    ])
  })

  it('repairs the saved selection when the selected engine is disabled', async () => {
    const store = await loadStore()
    const { actions } = store.getState()
    const id = getSearchEngineId(web)
    actions.setSearchEngineEnabled(id, true)
    actions.setLastUsedSearchEngine(id)
    page.handler.setLastUsedSearchEngine.mockClear()

    actions.setSearchEngineEnabled(id, false)
    expect(store.getState().lastUsedSearchEngine).toBe(brave.host)
    expect(page.handler.setLastUsedSearchEngine).toHaveBeenCalledWith(
      brave.host,
    )

    actions.setSearchEngineEnabled(id, true)
    expect(store.getState().lastUsedSearchEngine).toBe(brave.host)
  })
})
