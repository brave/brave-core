// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Button from '$web-components/button'
import { useBraveNews } from './Context'
import DiscoverSection from './DiscoverSection'
import FeedCard from './FeedCard'
import { getLocale } from '$web-common/locale'
import { api } from '../../../../api/brave_news/news'

const LoadMoreButtonContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: stretch;
  grid-column: 2;
`

const DEFAULT_SUGGESTIONS_COUNT = 3

export default function Suggestions () {
  const { filteredPublisherIds, publishers } = useBraveNews()
  const [showAll, setShowAll] = React.useState(false)
  const popularPublisherIds = React.useMemo(() => filteredPublisherIds.map(id => publishers[id])
    .map(p => [p, p.locales.find(l => l.locale === api.locale)?.rank] as const)
     // Filter out publishers which aren't in the current locale.
    .filter(([p, pRank]) => pRank !== undefined)
    .sort(([a, aRank], [b, bRank]) => {
      // Neither source has a rank, sort alphabetically
      if (!aRank && !bRank) {
        return a.publisherName.localeCompare(b.publisherName)
      }

      // 0 is considered unranked, because mojo doesn't support optional
      // primitives, so we want to sort 0 last.
      return (aRank || Number.MAX_SAFE_INTEGER) - (bRank || Number.MAX_SAFE_INTEGER)
    })
    .map(([p]) => p.publisherId), [filteredPublisherIds, publishers])

  const popularPublishersTruncated = React.useMemo(() => showAll
    ? popularPublisherIds
    : popularPublisherIds.slice(0, DEFAULT_SUGGESTIONS_COUNT), [popularPublisherIds, showAll])

  if (!popularPublisherIds.length) {
    return null
  }

  return (
    <DiscoverSection name={getLocale('braveNewsPopularTitle')}>
      {popularPublishersTruncated.map(s => <FeedCard key={s} publisherId={s} />)}
      {!showAll && popularPublisherIds.length > DEFAULT_SUGGESTIONS_COUNT &&
      <LoadMoreButtonContainer>
        <Button onClick={() => setShowAll(true)}>
          {getLocale('braveNewsShowMoreButton')}
        </Button>
      </LoadMoreButtonContainer>}
    </DiscoverSection>
  )
}
