// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import * as React from 'react'
import Flex from '$web-common/Flex'
import Carousel from './Carousel'
import { useBraveNews } from '../../../../../brave_news/browser/resources/shared/Context'
import CustomizeLink from './CustomizeLink'
import CustomizePage from './CustomizePage'
import DiscoverSection from './DiscoverSection'
import FeedCard from './FeedCard'

export function SuggestionsCarousel () {
  const { suggestedPublisherIds, setCustomizePage } = useBraveNews()

  return <Carousel
    title={<Flex justify='space-between'>
      {getLocale('braveNewsSuggestionsTitle')}
      <CustomizeLink onClick={() => setCustomizePage('suggestions')}>
        {getLocale('braveNewsViewAllButton')}
      </CustomizeLink>
    </Flex>}
    subtitle={getLocale('braveNewsSuggestionsSubtitle')}
    publisherIds={suggestedPublisherIds}/>
}

export function SuggestionsPage () {
  const { suggestedPublisherIds } = useBraveNews()
  return <CustomizePage title={getLocale('braveNewsSuggestionsTitle')}>
    <DiscoverSection>
      {suggestedPublisherIds.map(p => <FeedCard key={p} publisherId={p} />)}
    </DiscoverSection>
  </CustomizePage>
}
