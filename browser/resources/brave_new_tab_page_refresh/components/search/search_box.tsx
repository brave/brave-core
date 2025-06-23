/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import {
  SearchEngineInfo,
  AutocompleteMatch,
  ClickEvent,
  braveSearchHost } from '../../state/search_state'

import { useSearchState, useSearchActions } from '../../context/search_context'
import { urlFromInput } from '../../lib/url_input'
import { getString } from '../../lib/strings'
import { Popover } from '../common/popover'
import { EngineIcon } from './engine_icon'
import { SearchResults, ResultOption } from './search_results'
import classNames from '$web-common/classnames'

import { style } from './search_box.style'

interface Props {
  onCustomizeSearchEngineList: () => void
}

export function SearchBox(props: Props) {
  const actions = useSearchActions()

  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const searchEngines = useSearchState((s) => s.searchEngines)
  const enabledSearchEngines = useSearchState((s) => s.enabledSearchEngines)
  const defaultSearchEngine = useSearchState((s) => s.defaultSearchEngine)
  const lastUsedSearchEngine = useSearchState((s) => s.lastUsedSearchEngine)
  const searchMatches = useSearchState((s) => s.searchMatches)

  const inputRef = React.useRef<HTMLInputElement>(null)

  const [query, setQuery] = React.useState('')
  const [expanded, setExpanded] = React.useState(false)
  const [selectedOption, setSelectedOption] =
      React.useState<number | null>(null)
  const [showEngineOptions, setShowEngineOptions] = React.useState(false)
  const [currentEngine, setCurrentEngine] =
      React.useState(lastUsedSearchEngine || defaultSearchEngine)

  // If the enabled search engine list changes, and the current engine is no
  // longer in the list, then choose the default search engine. If the default
  // search engine is also not in the list, then choose the first engine in the
  // list of enabled engines.
  React.useEffect(() => {
    if (!enabledSearchEngines.has(currentEngine)) {
      if (enabledSearchEngines.size === 0 ||
          enabledSearchEngines.has(defaultSearchEngine)) {
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
      () => getResultOptions(query, searchMatches),
      [query, searchMatches])

  // When the result option list changes, select the first available option that
  // is allowed to be the default match.
  React.useEffect(() => {
    const optionSelected = resultOptions.some((option, index) => {
      if (option.kind === 'url' || option.match.allowedToBeDefaultMatch) {
        setSelectedOption(index)
        return true
      }
      return false
    })
    if (!optionSelected) {
      setSelectedOption(null)
    }
  }, [resultOptions])

  // Allow document styles to update based on expand state.
  React.useEffect(() => {
    document.body.classList.toggle('search-box-expanded', expanded)
  }, [expanded])

  // Report usage metrics when the search box is shown or hidden.
  React.useEffect(() => {
    if (showSearchBox && currentEngine) {
      actions.reportSearchEngineUsage(currentEngine)
    } else {
      actions.reportSearchBoxHidden()
    }
  }, [showSearchBox, currentEngine])

  const searchEngine =
      searchEngines.find(({ host }) => host === currentEngine)

  function updateQuery(query: string) {
    setQuery(query)
    if (query) {
      actions.queryAutocomplete(query, currentEngine)
    } else {
      actions.stopAutocomplete()
    }
  }

  function getPlaceholder() {
    if (currentEngine === braveSearchHost) {
      return getString('searchBoxPlaceholderTextBrave')
    }
    return getString('searchBoxPlaceholderText')
  }

  function focusInput() {
    inputRef.current?.focus()
  }

  function onSelectSearchEngine(engine: SearchEngineInfo) {
    return () => {
      setCurrentEngine(engine.host)
      actions.setLastUsedSearchEngine(engine.host)
      actions.stopAutocomplete()
      if (query) {
        actions.queryAutocomplete(query, engine.host)
      }
      setShowEngineOptions(false)
      focusInput()
    }
  }

  function onSearchClick(event: React.MouseEvent) {
    if (query) {
      actions.openSearch(query, currentEngine, event)
    }
  }

  function updateSelectedOption(step: number) {
    if (resultOptions.length === 0) {
      setSelectedOption(null)
      return
    }
    const defaultIndex = step > 0 ? -1 : 0
    let index = (selectedOption ?? defaultIndex) + step
    if (index < 0) {
      index = resultOptions.length - 1
    } else if (index >= resultOptions.length) {
      index = 0
    }
    setSelectedOption(index)
  }

  function onOptionClick(option: ResultOption, event: ClickEvent) {
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

  function onSearchSuggestionsEnabled() {
    updateQuery(query)
    focusInput()
  }

  function onKeyDown(event: React.KeyboardEvent) {
    if (event.key === 'Enter') {
      if (selectedOption !== null) {
        const option = resultOptions[selectedOption]
        onOptionClick(option, { ...event, button: 0 })
      } else if (query) {
        actions.openSearch(query, currentEngine, { ...event, button: 0 })
      }
      event.preventDefault()
    } else if (event.key === 'Escape') {
      updateQuery('')
    } else if (event.key === 'ArrowUp') {
      updateSelectedOption(-1)
      event.preventDefault()
    } else if (event.key === 'ArrowDown') {
      updateSelectedOption(1)
      event.preventDefault()
    }
  }

  function onInputContainerClick(event: React.MouseEvent) {
    if (event.target === event.currentTarget) {
      focusInput()
      setExpanded(true)
    }
  }

  function isSearchEngineEnabled(host: string) {
    if (enabledSearchEngines.size === 0) {
      return host === defaultSearchEngine
    }
    return enabledSearchEngines.has(host)
  }

  if (!searchFeatureEnabled) {
    return null
  }

  return (
    <div
      className={classNames({ expanded, hidden: !showSearchBox })}
      data-css-scope={style.scope}
    >
      <Popover
        isOpen={expanded}
        className='search-container'
        onClose={() => setExpanded(false)}
      >
        <div
          className='input-container'
          onClick={onInputContainerClick}
          onKeyDown={() => {}}
        >
          <input
            ref={inputRef}
            type='text'
            placeholder={getPlaceholder()}
            tabIndex={1}
            value={query}
            onClick={() => setExpanded(true)}
            onKeyDown={onKeyDown}
            onChange={(event) => {
              setExpanded(true)
              updateQuery(event.target.value)
            }}
          />
          <button
            className={classNames({
              'engine-picker-button': true,
              'open': showEngineOptions
            })}
            onClick={() => setShowEngineOptions(!showEngineOptions)}
          >
            {searchEngine && <EngineIcon engine={searchEngine} />}
          </button>
          <button className='search-button' onClick={onSearchClick}>
            <Icon name='search' />
          </button>
        </div>
        <div className='results-container'>
          <SearchResults
            options={resultOptions}
            selectedOption={selectedOption}
            onOptionClick={onOptionClick}
            onSearchSuggestionsEnabled={onSearchSuggestionsEnabled}
          />
        </div>
        <Popover
          className='engine-options'
          isOpen={showEngineOptions}
          onClose={() => setShowEngineOptions(false)}
        >
          <div className='popover-menu'>
            {searchEngines.map((engine) => {
              if (!isSearchEngineEnabled(engine.host)) {
                return null
              }
              return (
                <button
                  key={engine.host}
                  className={engine.host === currentEngine ? 'highlight' : ''}
                  onClick={onSelectSearchEngine(engine)}
                >
                  <EngineIcon engine={engine} />
                  {engine.name}
                </button>
              )
            })}
            <div className='divider' />
            <button onClick={props.onCustomizeSearchEngineList}>
              <Icon name='settings' />
              {getString('searchCustomizeEngineListText')}
            </button>
          </div>
        </Popover>
      </Popover>
    </div>
  )
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
