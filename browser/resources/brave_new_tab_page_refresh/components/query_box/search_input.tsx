/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { braveSearchHost } from '../../state/search_state'
import { useSearchState } from '../../context/search_context'
import { getString } from '../../lib/strings'
import { SearchEnginePicker } from '../search/search_engine_picker'
import { SearchResults } from '../search/search_results'
import { useSearchInputState } from '../search/search_input_state'

import { style } from './search_input.style'

interface Props {
  showSearchSettings: () => void
  onActivated: () => void
  renderInputControls?: () => React.ReactNode
}

export function SearchInput(props: Props) {
  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const showSearchBox = useSearchState((s) => s.showSearchBox)
  const inputState = useSearchInputState()
  const engineHost = inputState.searchEngine?.host
  const inputRef = React.useRef<HTMLInputElement>(null)

  function focusInput() {
    inputRef.current?.focus()
  }

  if (!searchFeatureEnabled || !showSearchBox) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <input
        ref={inputRef}
        autoFocus
        type='text'
        placeholder={
          engineHost === braveSearchHost
            ? getString('searchBoxPlaceholderTextBrave')
            : getString('searchBoxPlaceholderText')
        }
        value={inputState.query}
        onKeyDown={(event) => {
          inputState.handleActionKeyDown(event.nativeEvent)
        }}
        onChange={(event) => {
          props.onActivated()
          inputState.setQuery(event.target.value)
        }}
      />
      <div className='search-actions'>
        <SearchEnginePicker
          selectedEngine={inputState.searchEngine ?? null}
          searchEngines={inputState.searchEngineOptions}
          onSelectEngine={(engine) => {
            inputState.selectSearchEngine(engine)
            focusInput()
          }}
          onCustomizeClick={props.showSearchSettings}
        />
        {props.renderInputControls && props.renderInputControls()}
        <Button
          fab
          kind='filled'
          className='search-button'
          isDisabled={!inputState.query}
          onClick={inputState.openSearch}
        >
          <Icon name='arrow-up' />
        </Button>
      </div>
      <div className='results-container'>
        <SearchResults
          options={inputState.resultOptions}
          selectedOption={inputState.selectedResultOption}
          onOptionClick={inputState.openResultOption}
          onSearchSuggestionsEnabled={focusInput}
        />
      </div>
    </div>
  )
}
