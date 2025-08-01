// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import * as React from 'react'
import { useBraveNews } from '../shared/Context'
import Carousel from './Carousel'
import CustomizeLink from './CustomizeLink'
import CustomizePage from './CustomizePage'
import DiscoverSection from './DiscoverSection'
import PublisherCard from '../shared/PublisherCard'

const usePopularPublisherIds = () => {
  const { filteredPublisherIds, publishers, locale } = useBraveNews()
  return React.useMemo(() => filteredPublisherIds.map(id => publishers[id])
    .map(p => [p, p.locales.find(l => l.locale === locale)?.rank] as const)
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
    .map(([p]) => p.publisherId), [filteredPublisherIds, publishers, locale])
}

export function PopularCarousel () {
  const { setCustomizePage } = useBraveNews()
  const popularPublisherIds = usePopularPublisherIds()
  return (
    <Carousel title={<Flex justify='space-between'>
      {getLocale(S.BRAVE_NEWS_POPULAR_TITLE)}
      <CustomizeLink onClick={() => setCustomizePage('popular')}>
        {getLocale(S.BRAVE_NEWS_VIEW_ALL_BUTTON)}
      </CustomizeLink>
    </Flex>} publisherIds={popularPublisherIds} />
  )
}

export function PopularPage () {
  const popularPublisherIds = usePopularPublisherIds()
  return <CustomizePage title={getLocale(S.BRAVE_NEWS_POPULAR_TITLE)}>
    <DiscoverSection>
      {popularPublisherIds.map(p => <PublisherCard key={p} publisherId={p} />)}
    </DiscoverSection>
  </CustomizePage>
}
