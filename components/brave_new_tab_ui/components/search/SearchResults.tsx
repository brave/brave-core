// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { AutocompleteMatch, AutocompleteResult, OmniboxPopupSelection } from 'gen/ui/webui/resources/cr_components/searchbox/searchbox.mojom.m';
import * as React from 'react'
import styled from 'styled-components'
import SearchResult from './SearchResult'
import getNTPBrowserAPI, { SearchEngineInfo } from '../../api/background'
import { omniboxController, search, useSearchContext } from './SearchContext'
import { braveSearchHost } from './config'
import { stringToMojoString16 } from 'gen/ui/webui/resources/tsc/js/mojo_type_util';
import { handleOpenURLClick, validateScheme } from '$web-common/SecureLink';
import MaybePromptEnableSuggestions from './MaybePromptEnableSuggestions';

const Container = styled.div`
  border-top: 1px solid rgba(255, 255, 255, 0.1);

  background: rgba(255,255,255,0.1);

  color: ${color.white};

  border-bottom-left-radius: ${radius.m};
  border-bottom-right-radius: ${radius.m};

  padding: ${spacing.m};
  gap: ${spacing.s};
  display: flex;
  flex-direction: column;

  width: 540px;
  overflow: hidden;
  text-wrap: nowrap;
`

// Handles opening an autocomplete match, which may or may not be part of an
// autocomplete result.
const openMatch = (match: AutocompleteMatch, line: number, event: React.MouseEvent | KeyboardEvent, searchEngine?: SearchEngineInfo) => {
  if (line === -1) {
    handleOpenURLClick(match.destinationUrl.url, event)
    return
  }

  const button = 'button' in event ? event.button : 0
  if (searchEngine) {
    getNTPBrowserAPI().newTabMetrics.reportNTPSearchUsage(searchEngine.prepopulateId)
  }
  omniboxController.openAutocompleteMatch(line, match.destinationUrl, true, button, event.altKey, event.ctrlKey, event.metaKey, event.shiftKey)
}

const useUrlWhatYouTyped = (query: string) => React.useMemo(() => {
  try {
    // There should be at least one `.` to be a URL and no spaces.
    const bits = query.split('.')
    if (bits.length <= 1 || bits.some(b => !b) || query.includes(' ')) {
      return null;
    }

    // Force a scheme to be included - we validate whether the scheme is
    // allowed later.
    const q = query.includes('://') ? query : 'https://' + query
    const url = new URL(q)

    validateScheme(url.toString())

    return {
      destinationUrl: {
        url: url.toString()
      },
      contents: stringToMojoString16(url.toString()),
      description: stringToMojoString16(''),
      imageUrl: `chrome://favicon/size/64@1x/${q.toString()}`,
      allowedToBeDefaultMatch: true
    } as AutocompleteMatch
  } catch {
    return null
  }
}, [query])

export default function SearchResults() {
  const { query, searchEngine } = useSearchContext()
  const [result, setResult] = React.useState<AutocompleteResult>()

  // An optional result when a URL is entered.
  const urlWhatYouTyped = useUrlWhatYouTyped(query)

  // Filter out empty results (from urlWhatYouTyped) and the
  // 'search-what-you-typed', which always from from the default search engine.
  // eslint-disable-next-line @typescript-eslint/no-unnecessary-type-assertion
  const matches = React.useMemo(() => [urlWhatYouTyped!, ...(result?.matches ?? [])]
    .filter(r => r && r.type !== 'search-what-you-typed'), [urlWhatYouTyped, result])
  const [selectedMatch, setSelectedMatch] = React.useState<number>();

  React.useEffect(() => {
    search.addResultListener(setResult)
    return () => search.removeResultListener(setResult)
  }, [])

  React.useEffect(() => {
    setSelectedMatch(prev => {
      if (!matches.length) return undefined

      if (!prev) {
        const defaultMatchIndex = matches.findIndex(r => r.allowedToBeDefaultMatch)
        if (defaultMatchIndex !== -1) return defaultMatchIndex

        // Fall back to setting the first item as the selected match.
        return 0
      }

      if (prev >= matches.length) {
        return matches.length - 1
      }

      return prev
    })
  }, [matches])

  React.useEffect(() => {
    const listener = (selection: OmniboxPopupSelection) => setSelectedMatch(selection.line)
    search.addSelectionListener(listener)
    return () => {
      search.removeSelectionListener(listener)
    }
  }, [])

  React.useEffect(() => {
    const keyHandler = (e: KeyboardEvent) => {
      const handledKeys = ['ArrowUp', 'ArrowDown']
      if (!handledKeys.includes(e.key)) {
        return
      }

      e.preventDefault()

      const direction = e.key === 'ArrowUp' ? -1 : 1
      setSelectedMatch(s => {
        if (matches.length === 0) return undefined

        const start = s ?? -1
        const next = start + direction
        if (next < 0) return (matches.length - 1)
        if (next >= matches.length) return 0
        return next
      })
    }
    document.addEventListener('keydown', keyHandler)
    return () => {
      document.removeEventListener('keydown', keyHandler)
    }

  }, [result])

  React.useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.key !== 'Enter') return

      e.preventDefault()

      const match = matches[selectedMatch!]
      if (!match) {
        getNTPBrowserAPI().pageHandler.searchWhatYouTyped(searchEngine?.host ?? braveSearchHost, query, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey);
        return;
      }

      openMatch(match, result?.matches.indexOf(match) ?? -1, e, searchEngine)
    }
    document.addEventListener('keydown', handler)
    return () => {
      document.removeEventListener('keydown', handler)
    }
  }, [matches, selectedMatch, query, searchEngine])

  const onSearchResultClick = (match: AutocompleteMatch) => {
    const line = result?.matches.indexOf(match) ?? -1
    return (e: React.MouseEvent) => openMatch(match, line, e, searchEngine)
  }

  return matches.length ? <Container data-theme="dark" className='search-results'>
    <MaybePromptEnableSuggestions />
    {matches.map((r, i) => <SearchResult key={i} selected={i === selectedMatch} onClick={onSearchResultClick(r)} match={r} />)}
  </Container> : null
}
