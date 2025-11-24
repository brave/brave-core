/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from '$web-common/loadTimeData'
import { SearchBoxProxy } from './search_box_proxy'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { Store } from '../lib/store'
import { debounce } from '$web-common/debounce'

import {
  SearchState,
  SearchActions,
  SearchEngineInfo,
  defaultSearchActions } from './search_state'

const enabledSearchEnginesStorageKey = 'search-engines'

// Previous versions of the NTP used different values for the Google search
// engine host. If these values are found in local storage or preferences, they
// must be normalized to the proper host value.
const deprecatedGoogleSearchHosts = ['', 'google.com']
const googleSearchHost = 'www.google.com'

function loadEnabledSearchEngines(
  availableEngines: SearchEngineInfo[],
  defaultSearchEngine: string
) {
  const set = new Set([defaultSearchEngine])
  const data = localStorage.getItem(enabledSearchEnginesStorageKey)
  if (!data) {
    return set
  }
  let record: any = null
  try {
    record = JSON.parse(data)
  } catch {}
  if (!record || typeof record !== 'object') {
    return set
  }
  if (deprecatedGoogleSearchHosts.some((key) => record[key])) {
    record[googleSearchHost] = true
  }
  set.clear()
  for (const engine of availableEngines) {
    if (record[engine.host]) {
      set.add(engine.host)
    }
  }
  if (set.size === 0) {
    set.add(defaultSearchEngine)
  }
  return set
}

function storeEnabledSearchEngines(engines: Set<string>) {
  let record: Record<string, boolean> = {}
  for (const engine of engines) {
    record[engine] = true
  }
  localStorage.setItem(enabledSearchEnginesStorageKey, JSON.stringify(record))
}

export function createSearchHandler(
  store: Store<SearchState>
): SearchActions {
  if (!loadTimeData.getBoolean('ntpSearchFeatureEnabled')) {
    return defaultSearchActions()
  }

  const defaultSearchEngine = loadTimeData.getString('ntpSearchDefaultHost')
  const searchProxy = SearchBoxProxy.getInstance()
  const newTabProxy = NewTabPageProxy.getInstance()

  store.update({
    searchFeatureEnabled: true,
    defaultSearchEngine
  })

  async function updateSearchEngines() {
    const { searchEngines } =
        await newTabProxy.handler.getAvailableSearchEngines()

    store.update({
      searchEngines,
      enabledSearchEngines:
        loadEnabledSearchEngines(searchEngines, defaultSearchEngine)
    })
  }

  async function updatePrefs() {
    let [
      { showSearchBox },
      { enabled: searchSuggestionsEnabled },
      { dismissed: searchSuggestionsPromptDismissed },
      { engine: lastUsedSearchEngine }
    ] = await Promise.all([
      newTabProxy.handler.getShowSearchBox(),
      newTabProxy.handler.getSearchSuggestionsEnabled(),
      newTabProxy.handler.getSearchSuggestionsPromptDismissed(),
      newTabProxy.handler.getLastUsedSearchEngine()
    ])
    if (deprecatedGoogleSearchHosts.includes(lastUsedSearchEngine)) {
      lastUsedSearchEngine = googleSearchHost
    }
    store.update({
      showSearchBox,
      searchSuggestionsEnabled,
      searchSuggestionsPromptDismissed,
      lastUsedSearchEngine
    })
  }

  function findSearchEngine(engine: string) {
    return store.getState().searchEngines.find(({ host }) => host === engine)
  }

  searchProxy.addListeners({
    autocompleteResultChanged(result) {
      const searchMatches = result.matches.map((match) => {
        if (match.swapContentsAndDescription) {
          const { contents } = match
          match.contents = match.description
          match.description = contents
        }
        return match
      })
      store.update({ searchMatches })
    }
  })

  newTabProxy.addListeners({
    onSearchStateUpdated: debounce(updatePrefs, 10)
  })

  async function loadData() {
    await Promise.all([
      updateSearchEngines(),
      updatePrefs()
    ])

    store.update({ initialized: true })
  }

  loadData()

  return {

    setShowSearchBox(showSearchBox) {
      store.update({ showSearchBox })
      newTabProxy.handler.setShowSearchBox(showSearchBox)
    },

    setSearchSuggestionsEnabled(enabled) {
      store.update({ searchSuggestionsEnabled: enabled })
      newTabProxy.handler.setSearchSuggestionsEnabled(enabled)
    },

    setSearchSuggestionsPromptDismissed(dismissed) {
      store.update({ searchSuggestionsPromptDismissed: dismissed })
      newTabProxy.handler.setSearchSuggestionsPromptDismissed(dismissed)
    },

    setSearchEngineEnabled(engine, enabled) {
      store.update(({ enabledSearchEngines }) => {
        // Copy the set to ensure component state is updated.
        enabledSearchEngines = new Set(enabledSearchEngines)
        if (enabled) {
          enabledSearchEngines.add(engine)
        } else if (enabledSearchEngines.size > 1) {
          enabledSearchEngines.delete(engine)
        }
        storeEnabledSearchEngines(enabledSearchEngines)
        return { enabledSearchEngines }
      })
    },

    setLastUsedSearchEngine(engine) {
      store.update({ lastUsedSearchEngine: engine })
      newTabProxy.handler.setLastUsedSearchEngine(engine)
    },

    setActiveSearchInputKey(key) {
      const { activeSearchInputKey } = store.getState()
      if (key !== activeSearchInputKey) {
        searchProxy.handler.stopAutocomplete(true)
        store.update({
          activeSearchInputKey: key,
          searchMatches: []
        })
      }
    },

    queryAutocomplete(query, engine) {
      const searchEngine = findSearchEngine(engine)
      if (searchEngine && searchEngine.keyword) {
        query = [searchEngine.keyword, query].join(' ')
      }
      searchProxy.handler.queryAutocomplete(query, false)
    },

    openAutocompleteMatch(index, event) {
      if (index < 0) {
        return
      }
      const match = store.getState().searchMatches.at(index)
      if (!match) {
        return
      }
      searchProxy.handler.openAutocompleteMatch(
          index,
          match.destinationUrl,
          true,
          event.button,
          event.altKey,
          event.ctrlKey,
          event.metaKey,
          event.shiftKey)
    },

    stopAutocomplete() {
      searchProxy.handler.stopAutocomplete(true)
    },

    openSearch(query, engine, event) {
      newTabProxy.handler.openSearch(query, engine, event)
    },

    openUrlFromSearch(url, event) {
      newTabProxy.handler.openURLFromSearch(url, event)
    },

    reportSearchBoxHidden() {
      newTabProxy.handler.reportSearchBoxHidden();
    },

    reportSearchEngineUsage(engine) {
      const searchEngine = findSearchEngine(engine)
      if (searchEngine) {
        newTabProxy.handler.reportSearchEngineUsage(searchEngine.prepopulateId)
      }
    },

    reportSearchResultUsage(engine) {
      const searchEngine = findSearchEngine(engine)
      if (searchEngine) {
        newTabProxy.handler.reportSearchResultUsage(searchEngine.prepopulateId)
      }
    }
  }
}
