/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const braveSearchHost = 'search.brave.com'

export const defaultSearchEngine = braveSearchHost

export interface SearchEngineInfo {
  prepopulateId: bigint
  name: string
  keyword: string
  host: string
  faviconUrl: string
}

export interface SearchResultMatch {
  allowedToBeDefaultMatch: boolean
  contents: string
  description: string
  iconUrl: string
  imageUrl: string
  destinationUrl: string
}

export interface SearchState {
  searchFeatureEnabled: boolean
  showSearchBox: boolean
  searchEngines: SearchEngineInfo[]
  enabledSearchEngines: Set<string>
  lastUsedSearchEngine: string
  searchSuggestionsEnabled: boolean
  searchSuggestionsPromptDismissed: boolean
  searchMatches: SearchResultMatch[]
}

export function defaultSearchState(): SearchState {
  return {
    searchFeatureEnabled: false,
    showSearchBox: false,
    searchEngines: [],
    enabledSearchEngines: new Set(),
    lastUsedSearchEngine: '',
    searchSuggestionsEnabled: true,
    searchSuggestionsPromptDismissed: false,
    searchMatches: []
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
    queryAutocomplete(query, engine) {},
    openAutocompleteMatch(index, event) {},
    stopAutocomplete() {},
    openSearch(query, engine, event) {},
    openUrlFromSearch(url, event) {},
    reportSearchBoxHidden() {},
    reportSearchEngineUsage(engine) {},
    reportSearchResultUsage(engine) {}
  }
}
