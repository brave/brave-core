/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'

import { SearchModel, defaultModel, defaultState } from '../models/search_model'

export function createSearchModel(): SearchModel {
  const store = createStore({
    ...defaultState(),

    showSearchBox: true,

    searchSuggestionsEnabled: false,

    searchSuggestionsPromptDismissed: false,

    searchEngines: [{
      prepopulateId: BigInt(0),
      name: 'Brave',
      keyword: '',
      host: 'search.brave.com',
      faviconUrl: ''
    }, {
      prepopulateId: BigInt(1),
      name: 'Google',
      keyword: '',
      host: 'google.com',
      faviconUrl: ''
    }],

    enabledSearchEngines: new Set([
      'search.brave.com',
      'google.com'
    ])
  })

  return {
    ...defaultModel(),

    getState: store.getState,

    addListener: store.addListener,

    setShowSearchBox(showSearchBox) {
      store.update({ showSearchBox })
    },

    setSearchSuggestionsEnabled(enabled) {
      store.update({ searchSuggestionsEnabled: enabled })
    },

    setSearchSuggestionsPromptDismissed(dismissed) {
      store.update({ searchSuggestionsPromptDismissed: dismissed })
    },

    setLastUsedSearchEngine(engine) {
      store.update({ lastUsedSearchEngine: engine })
    },

    setSearchEngineEnabled(engine, enabled) {
      store.update(({ enabledSearchEngines }) => {
        enabledSearchEngines = new Set(enabledSearchEngines)
        if (enabled) {
          enabledSearchEngines.add(engine)
        } else if (enabledSearchEngines.size > 1) {
          enabledSearchEngines.delete(engine)
        }
        return { enabledSearchEngines }
      })
    },

    queryAutocomplete(query, engine) {
      store.update({
        searchMatches: [{
          allowedToBeDefaultMatch: false,
          contents: 'contents 1',
          description: 'description 1',
          iconUrl: '',
          imageUrl: '',
          destinationUrl: ''
        },
        {
          allowedToBeDefaultMatch: true,
          contents: 'contents 2',
          description: 'Ask Leo',
          iconUrl: '',
          imageUrl: '',
          destinationUrl: ''
        }]
      })
    },

    stopAutocomplete() {
      store.update({ searchMatches: [] })
    }
  }
}
