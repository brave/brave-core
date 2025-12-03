// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import { getString } from '../strings'
import * as React from 'react'
import { useBraveNews } from '../shared/Context'
import Carousel from './Carousel'
import CustomizeLink from './CustomizeLink'
import CustomizePage from './CustomizePage'
import DiscoverSection from './DiscoverSection'
import PublisherCard from '../shared/PublisherCard'

export function SuggestionsCarousel () {
  const { suggestedPublisherIds, setCustomizePage } = useBraveNews()

  return <Carousel
    title={<Flex justify='space-between'>
      {getString(S.BRAVE_NEWS_SUGGESTIONS_TITLE)}
      <CustomizeLink onClick={() => setCustomizePage('suggestions')}>
        {getString(S.BRAVE_NEWS_VIEW_ALL_BUTTON)}
      </CustomizeLink>
    </Flex>}
    subtitle={getString(S.BRAVE_NEWS_SUGGESTIONS_SUBTITLE)}
    publisherIds={suggestedPublisherIds}/>
}

export function SuggestionsPage () {
  const { suggestedPublisherIds } = useBraveNews()
  return <CustomizePage title={getString(S.BRAVE_NEWS_SUGGESTIONS_TITLE)}>
    <DiscoverSection>
      {suggestedPublisherIds.map(p => <PublisherCard key={p} publisherId={p} />)}
    </DiscoverSection>
  </CustomizePage>
}
