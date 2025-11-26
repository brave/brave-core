/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'chrome://resources/mojo/components/omnibox/browser/searchbox.mojom-webui.js'

export const braveSearchHost = 'search.brave.com'

export interface SearchEngineInfo {
  prepopulateId: bigint
  name: string
  keyword: string
  host: string
  faviconUrl: string
}

// In order to isolate ourselves from the large number of properties in the
// mojo type for testing/mocking purposes, we pick out only the fields that we
// require.
export type AutocompleteMatch = Pick<
  mojom.AutocompleteMatch,
  | 'allowedToBeDefaultMatch'
  | 'contents'
  | 'description'
  | 'iconUrl'
  | 'imageUrl'
  | 'destinationUrl'
>

export interface SearchState {
  initialized: boolean
  defaultSearchEngine: string
  searchFeatureEnabled: boolean
  showSearchBox: boolean
  searchEngines: SearchEngineInfo[]
  enabledSearchEngines: Set<string>
  lastUsedSearchEngine: string
  searchSuggestionsEnabled: boolean
  searchSuggestionsPromptDismissed: boolean
  activeSearchInputKey: string
  searchMatches: AutocompleteMatch[]
}

export function defaultSearchState(): SearchState {
  return {
    initialized: false,
    defaultSearchEngine: braveSearchHost,
    searchFeatureEnabled: false,
    showSearchBox: false,
    searchEngines: [],
    enabledSearchEngines: new Set(),
    lastUsedSearchEngine: '',
    searchSuggestionsEnabled: true,
    searchSuggestionsPromptDismissed: false,
    activeSearchInputKey: '',
    searchMatches: [],
  }
}

export interface ClickEvent {
  button: number
  altKey: boolean
  ctrlKey: boolean
  metaKey: boolean
  shiftKey: boolean
}

export interface SearchActions {
  setShowSearchBox: (showSearchBox: boolean) => void
  setSearchSuggestionsEnabled: (enabled: boolean) => void
  setSearchSuggestionsPromptDismissed: (dismissed: boolean) => void
  setLastUsedSearchEngine: (engine: string) => void
  setSearchEngineEnabled: (engine: string, enabled: boolean) => void
  setActiveSearchInputKey: (key: string) => void
  queryAutocomplete: (query: string, engine: string) => void
  openAutocompleteMatch: (index: number, event: ClickEvent) => void
  stopAutocomplete: () => void
  openSearch: (query: string, engine: string, event: ClickEvent) => void
  openUrlFromSearch: (url: string, event: ClickEvent) => void
  reportSearchBoxHidden: () => void
  reportSearchEngineUsage: (engine: string) => void
  reportSearchResultUsage: (engine: string) => void
}

export function defaultSearchActions(): SearchActions {
  return {
    setShowSearchBox(showSearchBox) {},
    setSearchSuggestionsEnabled(enabled) {},
    setSearchSuggestionsPromptDismissed(dismissed) {},
    setSearchEngineEnabled(engine, enabled) {},
    setLastUsedSearchEngine(engine) {},
    setActiveSearchInputKey(key) {},
    queryAutocomplete(query, engine) {},
    openAutocompleteMatch(index, event) {},
    stopAutocomplete() {},
    openSearch(query, engine, event) {},
    openUrlFromSearch(url, event) {},
    reportSearchBoxHidden() {},
    reportSearchEngineUsage(engine) {},
    reportSearchResultUsage(engine) {},
  }
}
