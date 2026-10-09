/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { SecondaryTextPlacement } from 'chrome://resources/mojo/components/omnibox/browser/suggest_template_info.mojom-webui.js'
import { SearchActions, defaultSearchStore } from '../state/search_store'

function suggestTemplate(primaryText: string, secondaryText: string) {
  return {
    primaryText,
    primaryTextClass: [],
    secondaryText,
    secondaryTextClass: [],
    secondaryTextPlacement: SecondaryTextPlacement.kUnspecified,
    image: null,
  }
}

export function createSearchStore() {
  const store = defaultSearchStore()

  store.update({
    initialized: true,

    searchFeatureEnabled: true,

    showChatInput: true,
    showSearchBox: true,

    searchSuggestionsEnabled: false,

    searchSuggestionsPromptDismissed: false,

    searchEngines: [
      {
        prepopulateId: BigInt(0),
        name: 'Brave',
        keyword: '',
        host: 'search.brave.com',
        faviconUrl: '',
      },
      {
        prepopulateId: BigInt(1),
        name: 'Google',
        keyword: '',
        host: 'google.com',
        faviconUrl: '',
      },
    ],

    enabledSearchEngines: new Set(['search.brave.com', 'google.com']),
  })

  const actions: SearchActions = {
    ...store.getState().actions,

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

    setActiveSearchInputKey(key) {
      const { activeSearchInputKey } = store.getState()
      if (key !== activeSearchInputKey) {
        store.update({
          activeSearchInputKey: key,
          searchMatches: [],
        })
      }
    },

    queryAutocomplete(query, engine) {
      store.update({
        searchMatches: [
          {
            allowedToBeDefaultMatch: false,
            iconUrl: '',
            suggestTemplate: suggestTemplate('contents 1', 'description 1'),
            destinationUrl: '',
          },
          {
            allowedToBeDefaultMatch: true,
            iconUrl: '',
            suggestTemplate: suggestTemplate('contents 2', 'Ask Leo'),
            destinationUrl: '',
          },
          {
            allowedToBeDefaultMatch: true,
            iconUrl: '',
            suggestTemplate: suggestTemplate(query, engine),
            destinationUrl: '',
          },
        ],
      })
    },

    setShowChatInput(showChatInput) {
      store.update({ showChatInput })
    },

    stopAutocomplete() {
      store.update({ searchMatches: [] })
    },

    async getUrlFromSearchInput(query) {
      if (query.includes('.') && !/\s/.test(query)) {
        return `https://${query}`
      }
      return null
    },
  }

  store.update({ actions })

  return store
}
