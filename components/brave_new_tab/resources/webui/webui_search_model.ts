/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { stringToMojoString16, mojoString16ToString } from 'chrome://resources/js/mojo_type_util.js'

import { SearchBoxProxy } from './search_box_proxy'
import { NewTabPageProxy } from './new_tab_page_proxy'
import { createStore } from '../lib/store'

import {
  SearchModel,
  SearchEngineInfo,
  defaultSearchEngine,
  defaultState } from '../models/search_model'

const enabledSearchEnginesStorageKey = 'search-engines'

function loadEnabledSearchEngines(availableEngines: SearchEngineInfo[]) {
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

export function createSearchModel(): SearchModel {
  const searchProxy = SearchBoxProxy.getInstance()
  const newTabProxy = NewTabPageProxy.getInstance()
  const store = createStore(defaultState())

  async function updateSearchEngines() {
    const { searchEngines } =
      await newTabProxy.handler.getAvailableSearchEngines()

    store.update({
      searchEngines,
      enabledSearchEngines: loadEnabledSearchEngines(searchEngines)
    })
  }

  async function updatePrefs() {
    const [
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
    store.update({
      showSearchBox,
      searchSuggestionsEnabled,
      searchSuggestionsPromptDismissed,
      lastUsedSearchEngine
    })
  }

  searchProxy.addListeners({
    autocompleteResultChanged(result) {
      const searchMatches = result.matches.map((m) => {
        const match = {
          allowedToBeDefaultMatch: m.allowedToBeDefaultMatch,
          contents: mojoString16ToString(m.contents),
          description: mojoString16ToString(m.description),
          iconUrl: m.iconUrl,
          imageUrl: m.imageUrl,
          destinationUrl: m.destinationUrl.url
        }

        if (m.swapContentsAndDescription) {
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
    onSearchPrefsUpdated() {
      updatePrefs()
    }
  })

  async function loadData() {
    await Promise.all([
      updateSearchEngines(),
      updatePrefs()
    ])
  }

  loadData()

  return {
    getState: store.getState,

    addListener: store.addListener,

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

    queryAutocomplete(query, engine) {
      const { searchEngines } = store.getState()
      const searchEngine = searchEngines.find(({ host }) => host === engine)
      if (searchEngine && searchEngine.keyword) {
        query = [searchEngine.keyword, query].join(' ')
      }
      searchProxy.handler.queryAutocomplete(stringToMojoString16(query), false)
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
        { url: match.destinationUrl },
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
    }
  }
}
