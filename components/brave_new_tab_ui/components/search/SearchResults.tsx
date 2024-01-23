// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { color, radius, spacing } from '@brave/leo/tokens/css'
import { AutocompleteResult, OmniboxPopupSelection } from 'gen/components/omnibox/browser/omnibox.mojom.m'
import * as React from 'react'
import styled from 'styled-components'
import { omniboxController, search } from './SearchBox'
import SearchResult from './SearchResult'

const Container = styled.div`
  margin-top: ${spacing.m};
  background: ${color.container.background};
  border-radius: ${radius.m};
  padding: ${spacing.m} 0;
  gap: ${spacing.m};
  display: flex;
  flex-direction: column;

  width: 540px;
  overflow: hidden;
  text-wrap: nowrap;
`

export default function SearchResults() {
  const [result, setResult] = React.useState<AutocompleteResult>()
  const [selectedMatch, setSelectedMatch] = React.useState<number>();

  React.useEffect(() => {
    const listener = (result?: AutocompleteResult) => {
      setResult(result)
      setSelectedMatch(prev => {
        if (!result) return undefined

        if (!prev) {
          const defaultMatchIndex = result.matches.findIndex(r => r.allowedToBeDefaultMatch)
          if (defaultMatchIndex !== -1) return defaultMatchIndex

          return undefined
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
    const listener = (selection: OmniboxPopupSelection) => {
      console.log("Set selection:", selection)
      setSelectedMatch(selection.line)
    }
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
      if (!match) return

      omniboxController.openAutocompleteMatch(selectedMatch!, match.destinationUrl, true, 0, e.altKey, e.ctrlKey, e.metaKey, e.shiftKey);
    }
    document.addEventListener('keydown', handler)
    return () => {
      document.removeEventListener('keydown', handler)
    }
  }, [result, selectedMatch])
  return result && result?.matches.length ? <Container>
    {result?.matches.map((r, i) => <SearchResult key={i} selected={i === selectedMatch} line={i} match={r} />)}
  </Container> : null
}
