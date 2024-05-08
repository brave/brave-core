// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color, radius, spacing } from '@brave/leo/tokens/css/variables'
import { AutocompleteResult, OmniboxPopupSelection } from 'gen/ui/webui/resources/cr_components/searchbox/searchbox.mojom.m';
import * as React from 'react'
import styled from 'styled-components'
import SearchResult from './SearchResult'
import getNTPBrowserAPI from '../../api/background'
import { omniboxController, search, useSearchContext } from './SearchContext'
import { braveSearchHost } from './config'

const Container = styled.div`
  border-top: 1px solid ${color.divider.subtle};

  background: rgba(255,255,255,0.1);

  color: ${color.white};

  border-bottom-left-radius: ${radius.m};
  border-bottom-right-radius: ${radius.m};

  padding: ${spacing.m} 0;
  gap: ${spacing.m};
  display: flex;
  flex-direction: column;

  width: 540px;
  overflow: hidden;
  text-wrap: nowrap;
`

export default function SearchResults() {
  const { query, searchEngine } = useSearchContext()
  const [result, setResult] = React.useState<AutocompleteResult>()
  const [selectedMatch, setSelectedMatch] = React.useState<number>();

  React.useEffect(() => {
    const listener = (result?: AutocompleteResult) => {
      // The autocomplete provider generates a 'search-what-you-typed' result which includes
      // the keyword (and is always for the default search engine). We filter it out, as it
      // makes the results neater.
      if (result) {
        result.matches = result.matches.filter(r => r.type !== 'search-what-you-typed')
      }
      setResult(result)
      setSelectedMatch(prev => {
        if (!result) return undefined

        if (!prev) {
          const defaultMatchIndex = result.matches.findIndex(r => r.allowedToBeDefaultMatch)
          if (defaultMatchIndex !== -1) return defaultMatchIndex

          // Fall back to setting the first item as the selected match.
          return 0
        }

        if (prev >= result.matches.length) {
          return result.matches.length - 1
        }

        return prev
      })
    }
    search.addResultListener(listener)
    return () => search.removeResultListener(listener)
  }, [])

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
        if (!result || result.matches.length === 0) return undefined

        const start = s ?? -1
        const next = start + direction
        if (next < 0) return (result.matches.length - 1)
        if (next >= result.matches.length) return 0
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

      const match = result?.matches[selectedMatch!]
      if (!match) {
        getNTPBrowserAPI().pageHandler.searchWhatYouTyped(searchEngine?.host ?? braveSearchHost, query, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey);
        return;
      }

      omniboxController.openAutocompleteMatch(selectedMatch!, match.destinationUrl, true, 0, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey);
    }
    document.addEventListener('keydown', handler)
    return () => {
      document.removeEventListener('keydown', handler)
    }
  }, [result, selectedMatch, query, searchEngine])
  return result && result?.matches.length ? <Container data-theme="dark" className='search-results'>
    {result?.matches.map((r, i) => <SearchResult key={i} selected={i === selectedMatch} line={i} match={r} />)}
  </Container> : null
}
