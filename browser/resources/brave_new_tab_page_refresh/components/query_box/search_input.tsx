/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { braveSearchHost } from '../../state/search_store'
import { getString } from '../../lib/strings'
import { SearchEnginePicker } from '../search/search_engine_picker'
import { SearchResults } from '../search/search_results'
import { useSearchInputState } from '../search/search_input_state'

import { style } from './search_input.style'

interface Props {
  showSearchSettings: () => void
  renderInputToggle?: () => React.ReactNode
}

// Search box for the new tab page. Renders the input and controls in two rows
// when an input toggle is provided, and in a single row otherwise.
export function SearchInput(props: Props) {
  const inputState = useSearchInputState('search-box')
  const engineHost = inputState.searchEngine?.host
  const inputRef = React.useRef<HTMLInputElement>(null)
  const singleLine = !props.renderInputToggle

  function focusInput() {
    inputRef.current?.focus()
  }

  const input = (
    <input
      ref={inputRef}
      autoFocus
      type='text'
      placeholder={
        engineHost === braveSearchHost
          ? getString(S.NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT_BRAVE)
          : getString(S.NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT)
      }
      value={inputState.query}
      onFocus={() => inputState.setActiveInput()}
      onKeyDown={(event) => {
        inputState.handleActionKeyDown(event.nativeEvent)
      }}
      onChange={(event) => {
        inputState.setQuery(event.target.value)
      }}
    />
  )

  const enginePicker = (
    <SearchEnginePicker
      selectedEngine={inputState.searchEngine ?? null}
      searchEngines={inputState.searchEngineOptions}
      onSelectEngine={(engine) => {
        inputState.selectSearchEngine(engine)
        focusInput()
      }}
      onCustomizeClick={props.showSearchSettings}
    />
  )

  const searchButton = (
    <Button
      fab
      kind='filled'
      className='search-button'
      isDisabled={!inputState.query}
      onClick={inputState.openSearch}
    >
      <Icon name='arrow-up' />
    </Button>
  )

  return (
    <div
      data-css-scope={style.scope}
      data-testid='ntp-search-input'
      className={singleLine ? 'single-line' : undefined}
    >
      {singleLine ? (
        <div className='input-row'>
          {enginePicker}
          {input}
          {searchButton}
        </div>
      ) : (
        <div className='input-row'>
          {input}
          <div className='search-actions'>
            {enginePicker}
            <div className='toggle'>{props.renderInputToggle?.()}</div>
            {searchButton}
          </div>
        </div>
      )}
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
