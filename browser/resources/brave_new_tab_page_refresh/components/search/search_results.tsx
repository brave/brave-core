/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { AutocompleteMatch, ClickEvent } from '../../state/search_state'
import { useSearchState, useSearchActions } from '../../context/search_context'
import { getString } from '../../lib/strings'
import { faviconURL } from '../../lib/favicon_url'
import { SafeImage } from '../common/safe_image'

import { style } from './search_results.style'

function MatchImage(props: { match: AutocompleteMatch }) {
  const { imageUrl, iconUrl } = props.match

  if (props.match.description === getString(S.OMNIBOX_ASK_LEO_DESCRIPTION)) {
    return <Icon name='product-brave-leo' className='brave-leo-icon' />
  }
  if (!imageUrl) {
    if (!iconUrl.url) {
      return <Icon name='search' className='search-icon' />
    }
    return <img className='icon' src={iconUrl.url} />
  }
  if (imageUrl.startsWith('chrome:')) {
    return <img src={imageUrl} />
  }
  return <SafeImage src={imageUrl} />
}

function MatchText(props: { match: AutocompleteMatch }) {
  return <>
    {props.match.contents}
    <span className='description'>
      {props.match.description}
    </span>
  </>
}

interface URLResultOption {
  kind: 'url'
  url: string
}

interface MatchResultOption {
  kind: 'match'
  matchIndex: number
  match: AutocompleteMatch
}

export type ResultOption = URLResultOption | MatchResultOption

interface Props {
  options: ResultOption[]
  selectedOption: number | null
  onOptionClick: (option: ResultOption, event: ClickEvent) => void
  onSearchSuggestionsEnabled: () => void
}

export function SearchResults(props: Props) {
  const { selectedOption, options } = props

  const actions = useSearchActions()

  const searchSuggestionsEnabled =
    useSearchState((s) => s.searchSuggestionsEnabled)
  const searchSuggestionsPromptDismissed =
    useSearchState((s) => s.searchSuggestionsPromptDismissed)

  if (options.length === 0) {
    return null
  }

  function renderSearchSuggestionsPrompt() {
    if (searchSuggestionsEnabled || searchSuggestionsPromptDismissed) {
      return null
    }
    return (
      <div className='suggestions-prompt'>
        <h4>{getString(S.NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TITLE)}</h4>
        <p>
          {getString(S.NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TEXT)}
        </p>
        <div className='actions'>
          <Button
            onClick={() => {
              actions.setSearchSuggestionsEnabled(true)
              props.onSearchSuggestionsEnabled()
            }}
          >
            {getString(S.NEW_TAB_SEARCH_SUGGESTIONS_ENABLE_BUTTON_LABEL)}
          </Button>
          <Button
            kind='plain-faint'
            onClick={() => {
              actions.setSearchSuggestionsPromptDismissed(true)
            }}
          >
            {getString(S.NEW_TAB_SEARCH_SUGGESTIONS_DISMISS_BUTTON_LABEL)}
          </Button>
        </div>
      </div>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      {renderSearchSuggestionsPrompt()}
      <div className='result-options'>
        {options.map((option, index) => {
          const isSelected = (selectedOption ?? -1) === index
          const className = isSelected ? 'selected' : ''
          const onClick = (event: React.MouseEvent) => {
            props.onOptionClick(option, event)
          }

          if (option.kind === 'url') {
            return (
              <button key={option.url} className={className} onClick={onClick}>
                <span className='result-image'>
                  <img className='favicon' src={faviconURL(option.url)} />
                </span>
                <span className='content'>
                  {option.url}
                </span>
              </button>
            )
          }

          const { match } = option

          return (
            <button
              key={option.matchIndex}
              className={className}
              onClick={onClick}
            >
              <span className='result-image'>
                <MatchImage match={match} />
              </span>
              <span className='content'>
                <MatchText match={match} />
              </span>
            </button>
          )
        })}
      </div>
    </div>
  )
}
