/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { mojoString16ToString } from 'chrome://resources/js/mojo_type_util.js'

import { AutocompleteMatch, ClickEvent } from '../../models/search'
import { getString } from '../../lib/strings'
import { useAppActions, useAppState } from '../context/app_model_context'
import { placeholderImageSrc } from '../../lib/image_loader'
import { faviconURL } from '../../lib/favicon_url'
import { Optional } from '../../lib/optional'
import { SafeImage } from '../common/safe_image'

import { style } from './search_results.style'

function useMojoString16<T>(value: T) {
  return React.useMemo(() => mojoString16ToString(value), [value])
}

function MatchImage(props: { match: AutocompleteMatch }) {
  const { imageUrl, iconUrl } = props.match
  const description = useMojoString16(props.match.description)

  if (description === getString('searchAskLeoDescription')) {
    return <Icon name='product-brave-leo' className='brave-leo-icon' />
  }
  if (!imageUrl) {
    return <img className='icon' src={iconUrl || placeholderImageSrc} />
  }
  if (imageUrl.startsWith('chrome:')) {
    return <img src={imageUrl} />
  }
  return <SafeImage src={imageUrl} />
}

function MatchText(props: { match: AutocompleteMatch }) {
  const contents = useMojoString16(props.match.contents)
  const description = useMojoString16(props.match.description)
  return <>
    {contents}
    <span className='description'>
      {description}
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
  selectedOption: Optional<number>
  onOptionClick: (option: ResultOption, event: ClickEvent) => void
  onSearchSuggestionsEnabled: () => void
}

export function SearchResults(props: Props) {
  const { selectedOption, options } = props

  const actions = useAppActions()

  const searchSuggestionsEnabled =
      useAppState((s) => s.searchSuggestionsEnabled)
  const searchSuggestionsPromptDismissed =
      useAppState((s) => s.searchSuggestionsPromptDismissed)

  if (options.length === 0) {
    return null
  }

  function renderSearchSuggestionsPrompt() {
    if (searchSuggestionsEnabled || searchSuggestionsPromptDismissed) {
      return null
    }
    return (
      <div className='suggestions-prompt'>
        <h4>{getString('searchSuggestionsPromptTitle')}</h4>
        <p>
          {getString('searchSuggestionsPromptText')}
        </p>
        <div className='actions'>
          <Button
            onClick={() => {
              actions.setSearchSuggestionsEnabled(true)
              props.onSearchSuggestionsEnabled()
            }}
          >
            {getString('searchSuggestionsEnableButtonLabel')}
          </Button>
          <Button
            kind='plain-faint'
            onClick={() => {
              actions.setSearchSuggestionsPromptDismissed(true)
            }}
          >
            {getString('searchSuggestionsDismissButtonLabel')}
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
          const isSelected = selectedOption.valueOr(-1) === index
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
