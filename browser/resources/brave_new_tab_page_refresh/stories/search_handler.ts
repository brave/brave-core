/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { stringToMojoString16 } from 'chrome://resources/js/mojo_type_util.js'

import { Store } from '../lib/store'
import { SearchState, SearchActions, defaultSearchActions } from '../state/search_state'

export function createSearchHandler(store: Store<SearchState>): SearchActions {
  store.update({
    searchFeatureEnabled: true,

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

    ...defaultSearchActions(),

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
          contents: stringToMojoString16('contents 1'),
          description: stringToMojoString16('description 1'),
          iconUrl: { url: '' },
          imageUrl: '',
          destinationUrl: { url: '' }
        },
        {
          allowedToBeDefaultMatch: true,
          contents: stringToMojoString16('contents 2'),
          description: stringToMojoString16('Ask Leo'),
          iconUrl: { url: '' },
          imageUrl: '',
          destinationUrl: { url: '' }
        }]
      })
    },

    stopAutocomplete() {
      store.update({ searchMatches: [] })
    }
  }
}
