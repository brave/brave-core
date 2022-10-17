// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import Button from '$web-components/button'
import TextInput from '$web-components/input'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import { useBraveNews, useChannels } from './Context'
import Flex from '../../../Flex'
import ChannelCard from './ChannelCard'
import DiscoverSection from './DiscoverSection'
import FeedCard, { DirectFeedCard } from './FeedCard'
import useSearch from './useSearch'

const Header = styled.span`
  font-size: 24px;
  font-weight: 600;
  padding: 12px 0;
`

const SearchInput = styled(TextInput)`
  margin: 16px 0;
  border-radius: 4px;
  --interactive8: #AEB1C2;
  --focus-border: #737ADE;
`

const LoadMoreButtonContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: stretch;
  grid-column: 2;
`

// The default number of category cards to show.
const DEFAULT_NUM_CATEGORIES = 3

export default function Discover () {
  const [query, setQuery] = useState('')

  return <Flex direction='column'>
    <Header>Discover</Header>
    <SearchInput type="search" placeholder={getLocale('braveNewsSearchPlaceholderLabel')} value={query} onChange={e => setQuery(e.currentTarget.value)} />
    { query.length
      ? <SearchResults query={query} />
      : <Home />
    }
  </Flex>
}

function Home () {
  const [showingAllCategories, setShowingAllCategories] = React.useState(false)
  const channels = useChannels()
  const { filteredPublisherIds } = useBraveNews()

  const visibleChannelIds = React.useMemo(() => channels
    // If we're showing all channels, there's no end to the slice.
    // Otherwise, just show the default number.
    .slice(0, showingAllCategories
      ? undefined
      : DEFAULT_NUM_CATEGORIES)
    .map(c => c.channelName),
    [channels, showingAllCategories])

  return (
    <>
      <DiscoverSection name={getLocale('braveNewsChannelsHeader')}>
      {visibleChannelIds.map(channelId =>
        <ChannelCard key={channelId} channelId={channelId} />
      )}
      {!showingAllCategories && <LoadMoreButtonContainer>
        <Button onClick={() => setShowingAllCategories(true)}>
            {getLocale('braveNewsLoadMoreCategoriesButton')}
        </Button>
      </LoadMoreButtonContainer>}
      </DiscoverSection>
      <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
      {filteredPublisherIds.map(publisherId =>
        <FeedCard key={publisherId} publisherId={publisherId} />
      )}
      </DiscoverSection>
    </>
  )
}

interface SearchResultsProps {
  query: string
}
function SearchResults (props: SearchResultsProps) {
  const search = useSearch(props.query)
  const isFetchable = (search.feedUrlQuery !== null)
  const showFetchPermissionButton = (isFetchable && (!search.canFetchUrl || search.loading))

  const hasAnyChannels = search.filteredChannels.length > 0
  const hasAnySources = (search.filteredSources.publisherIds.length > 0 || search.filteredSources.direct.length > 0)

  return (
    <>
      {hasAnyChannels &&
      <DiscoverSection name={getLocale('braveNewsChannelsHeader')}>
        {search.filteredChannels.map(c =>
          <ChannelCard key={c.channelName} channelId={c.channelName} />
        )}
      </DiscoverSection>
      }
      <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
        {search.filteredSources.publisherIds.map(publisherId =>
          <FeedCard key={publisherId} publisherId={publisherId} />
        )}
        {showFetchPermissionButton &&
          <div>
            <Button scale='tiny' onClick={() => search.setCanFetchUrl(true)} isLoading={search.loading}>
              {getLocale('braveNewsDirectSearchButton').replace('$1', search.feedUrlQuery ?? '')}
            </Button>
          </div>
        }
        {search.filteredSources.direct.map(r =>
          <DirectFeedCard key={r.feedUrl.url} feedUrl={r.feedUrl.url} title={r.feedTitle} />)}
        {!search.canQueryFilterSources &&
          getLocale('braveNewsSearchQueryTooShort')
        }
        {isFetchable && !hasAnySources && !showFetchPermissionButton &&
          getLocale('braveNewsDirectSearchNoResults').replace('$1', search.feedUrlQuery ?? '')
        }
      </DiscoverSection>
    </>
  )
}
