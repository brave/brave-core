/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { braveSearchHost } from '../../state/search_state'

import { useSearchState } from '../../context/search_context'
import { useSearchInputState } from './search_input_state'
import { getString } from '../../lib/strings'
import { Popover } from '../common/popover'
import { SearchEnginePicker } from './search_engine_picker'
import { SearchResults } from './search_results'
import classNames from '$web-common/classnames'

import { style } from './search_box.style'

interface Props {
  showSearchSettings: () => void
}

export function SearchBox(props: Props) {
  const inputState = useSearchInputState('search-box')
  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const [expanded, setExpanded] = React.useState(false)
  const inputRef = React.useRef<HTMLInputElement>(null)

  // Allow document styles to update based on expand state.
  React.useEffect(() => {
    document.body.classList.toggle('search-box-expanded', expanded)
  }, [expanded])

  const engineHost = inputState.searchEngine?.host

  function focusInput() {
    inputRef.current?.focus()
  }

  function onInputContainerClick(event: React.MouseEvent) {
    if (event.target === event.currentTarget) {
      focusInput()
      setExpanded(true)
    }
  }

  if (!searchFeatureEnabled || !showSearchBox) {
    return null
  }

  return (
    <div className={classNames({ expanded })} data-css-scope={style.scope}>
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
            placeholder={
              engineHost === braveSearchHost
                ? getString(S.NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT_BRAVE)
                : getString(S.NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT)
            }
            tabIndex={1}
            value={inputState.query}
            onFocus={() => inputState.setActiveInput()}
            onClick={() => setExpanded(true)}
            onKeyDown={(event) => {
              inputState.handleActionKeyDown(event.nativeEvent)
            }}
            onChange={(event) => {
              setExpanded(true)
              inputState.setQuery(event.target.value)
            }}
          />
          <SearchEnginePicker
            selectedEngine={inputState.searchEngine ?? null}
            searchEngines={inputState.searchEngineOptions}
            onSelectEngine={(engine) => {
              inputState.selectSearchEngine(engine)
              focusInput()
            }}
            onCustomizeClick={props.showSearchSettings}
          />
          <button className='search-button' onClick={inputState.openSearch}>
            <Icon name='search' />
          </button>
        </div>
        <div className='results-container'>
          <SearchResults
            options={inputState.resultOptions}
            selectedOption={inputState.selectedResultOption}
            onOptionClick={inputState.openResultOption}
            onSearchSuggestionsEnabled={focusInput}
          />
        </div>
      </Popover>
    </div>
  )
}
