/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  SearchEngineInfo,
  AutocompleteMatch,
  ClickEvent,
} from '../../state/search_state'

import {
  useSearchState,
  useSearchActions,
  useSearchMatches,
} from '../../context/search_context'

import { ResultOption } from './search_results'
import { urlFromInput } from '../../lib/url_input'

export function useSearchInputState(inputKey: string) {
  const actions = useSearchActions()

  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const searchEngines = useSearchState((s) => s.searchEngines)
  const enabledSearchEngines = useSearchState((s) => s.enabledSearchEngines)
  const defaultSearchEngine = useSearchState((s) => s.defaultSearchEngine)
  const lastUsedSearchEngine = useSearchState((s) => s.lastUsedSearchEngine)
  const searchMatches = useSearchMatches(inputKey)
  const searchSuggestionsEnabled = useSearchState(
    (s) => s.searchSuggestionsEnabled,
  )

  const [query, setQuery] = React.useState('')
  const [selectedResultOption, setSelectedResultOption] = React.useState<
    number | null
  >(null)
  const [currentEngine, setCurrentEngine] = React.useState(
    lastUsedSearchEngine || defaultSearchEngine,
  )

  // If the enabled search engine list changes, and the current engine is no
  // longer in the list, then choose the default search engine. If the default
  // search engine is also not in the list, then choose the first engine in the
  // list of enabled engines.
  React.useEffect(() => {
    if (!enabledSearchEngines.has(currentEngine)) {
      if (
        enabledSearchEngines.size === 0
        || enabledSearchEngines.has(defaultSearchEngine)
      ) {
        setCurrentEngine(defaultSearchEngine)
      } else {
        const [firstEngine] = enabledSearchEngines.values()
        setCurrentEngine(firstEngine)
      }
    }
  }, [enabledSearchEngines, defaultSearchEngine])

  // Build the list of result options. The result options can contain a direct
  // URL (if the user has typed a URL) or a list of autocomplete options.
  const resultOptions = React.useMemo(
    () => getResultOptions(query, searchMatches ?? []),
    [query, searchMatches],
  )

  // When the result option list changes, select the first available option that
  // is allowed to be the default match.
  React.useEffect(() => {
    const optionSelected = resultOptions.some((option, index) => {
      if (option.kind === 'url' || option.match.allowedToBeDefaultMatch) {
        setSelectedResultOption(index)
        return true
      }
      return false
    })
    if (!optionSelected) {
      setSelectedResultOption(null)
    }
  }, [resultOptions])

  // Report usage metrics when the search box is shown or hidden.
  React.useEffect(() => {
    if (showSearchBox && currentEngine) {
      actions.reportSearchEngineUsage(currentEngine)
    } else {
      actions.reportSearchBoxHidden()
    }
  }, [showSearchBox, currentEngine])

  // Begin or cancel autocomplete queries when the query value or selected
  // search engine changes.
  React.useEffect(() => {
    if (query) {
      actions.queryAutocomplete(query, currentEngine)
    } else {
      actions.stopAutocomplete()
    }
  }, [query, currentEngine, searchSuggestionsEnabled])

  const searchEngine = searchEngines.find(({ host }) => host === currentEngine)

  const searchEngineOptions = searchEngines.filter((engine) => {
    if (enabledSearchEngines.size === 0) {
      return engine.host === defaultSearchEngine
    }
    return enabledSearchEngines.has(engine.host)
  })

  function selectSearchEngine(engine: SearchEngineInfo) {
    setCurrentEngine(engine.host)
    actions.setLastUsedSearchEngine(engine.host)
    actions.stopAutocomplete()
    if (query) {
      actions.queryAutocomplete(query, engine.host)
    }
  }

  function openSearch(event: ClickEvent) {
    if (query) {
      actions.openSearch(query, currentEngine, event)
    }
  }

  function updateSelectedOption(step: number) {
    if (resultOptions.length === 0) {
      setSelectedResultOption(null)
      return
    }
    const defaultIndex = step > 0 ? -1 : 0
    let index = (selectedResultOption ?? defaultIndex) + step
    if (index < 0) {
      index = resultOptions.length - 1
    } else if (index >= resultOptions.length) {
      index = 0
    }
    setSelectedResultOption(index)
  }

  function openResultOption(option: ResultOption, event: ClickEvent) {
    switch (option.kind) {
      case 'url': {
        actions.openUrlFromSearch(option.url, event)
        break
      }
      case 'match': {
        actions.reportSearchResultUsage(currentEngine)
        actions.openAutocompleteMatch(option.matchIndex, event)
        break
      }
    }
  }

  function setActiveInput() {
    actions.setActiveSearchInputKey(inputKey)
  }

  function handleActionKeyDown(event: KeyboardEvent) {
    if (event.key === 'Enter') {
      if (selectedResultOption !== null) {
        const option = resultOptions[selectedResultOption]
        openResultOption(option, { ...event, button: 0 })
      } else if (query) {
        actions.openSearch(query, currentEngine, { ...event, button: 0 })
      }
      event.preventDefault()
    } else if (event.key === 'Escape') {
      setQuery('')
    } else if (event.key === 'ArrowUp') {
      updateSelectedOption(-1)
      event.preventDefault()
    } else if (event.key === 'ArrowDown') {
      updateSelectedOption(1)
      event.preventDefault()
    }
  }

  return {
    query,
    setQuery,
    setActiveInput,
    handleActionKeyDown,
    openSearch,
    resultOptions,
    selectedResultOption,
    openResultOption,
    searchEngine,
    searchEngineOptions,
    selectSearchEngine,
  }
}

// Returns a list of `ResultOptions` for the specified query and corresponding
// autocomplete matches. In addition to the autocomplete matches, the list may
// also contain a URL match if the user typed in what appears to be a URL.
function getResultOptions(query: string, matches: AutocompleteMatch[]) {
  const options: ResultOption[] = []
  const inputURL = urlFromInput(query)
  if (inputURL) {
    let url = inputURL.toString()
    const index = url.lastIndexOf(query)
    if (index >= 0) {
      url = url.substring(0, index + query.length)
    }
    options.push({ kind: 'url', url })
  }
  matches.forEach((match, matchIndex) => {
    options.push({ kind: 'match', matchIndex, match })
  })
  return options
}
