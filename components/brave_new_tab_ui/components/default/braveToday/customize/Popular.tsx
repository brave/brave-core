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
    .filter(p => p.rank !== 0) // Rank of zero is unranked
    .sort((a, b) => a.rank - b.rank)
    .map(p => p.publisherId), [filteredPublisherIds, publishers])

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
