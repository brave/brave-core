// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import { getString } from '../strings'
import TextInput from '$web-components/input'
import Button from '@brave/leo/react/button'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { useBraveNews, useChannels } from '../shared/Context'
import ChannelCard from './ChannelCard'
import DiscoverSection from './DiscoverSection'
import PublisherCard, { DirectPublisherCard } from '../shared/PublisherCard'
import { PopularCarousel } from './Popular'
import { SuggestionsCarousel } from './Suggestions'
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

export default function Discover () {
  const [query, setQuery] = useState('')

  return <Flex direction='column'>
    <Header>Discover</Header>
    <SearchInput type="search" placeholder={getString(S.BRAVE_NEWS_SEARCH_PLACEHOLDER_LABEL)} value={query} onChange={e => setQuery(e.currentTarget.value)} />
    { query.length
      ? <SearchResults query={query} />
      : <Home />
    }
  </Flex>
}

function Home () {
  const channels = useChannels()
  const { updateSuggestedPublisherIds } = useBraveNews()

  const channelNames = React.useMemo(() => channels.map(c => c.channelName),
    [channels])

  // When we mount this component, update the suggested publisher ids.
  React.useEffect(() => { updateSuggestedPublisherIds() }, [])

  return (
    <>
      <PopularCarousel />
      <SuggestionsCarousel />
      <DiscoverSection name={getString(S.BRAVE_NEWS_BROWSE_CHANNELS_HEADER)}>
      {channelNames.map(channelName =>
        <ChannelCard key={channelName} channelName={channelName} />
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
      <DiscoverSection name={getString(S.BRAVE_NEWS_BROWSE_CHANNELS_HEADER)}>
        {search.filteredChannels.map(c =>
          <ChannelCard key={c.channelName} channelName={c.channelName} />
        )}
      </DiscoverSection>
      }
      <DiscoverSection name={getString(S.BRAVE_NEWS_ALL_SOURCES_HEADER)}>
        {search.filteredSources.publisherIds.map(publisherId =>
          <PublisherCard key={publisherId} publisherId={publisherId} />
        )}
        {showFetchPermissionButton &&
          <div>
            <Button size='tiny' kind='plain-faint' onClick={() => search.setCanFetchUrl(true)} isLoading={search.loading}>
              {getString(S.BRAVE_NEWS_DIRECT_SEARCH_BUTTON).replace('$1', search.feedUrlQuery ?? '')}
            </Button>
          </div>
        }
        {search.filteredSources.direct.map(r =>
          <DirectPublisherCard key={r.feedUrl.url} feedUrl={r.feedUrl.url} title={r.feedTitle} />)}
        {!search.canQueryFilterSources &&
          getString(S.BRAVE_NEWS_SEARCH_QUERY_TOO_SHORT)
        }
        {isFetchable && !hasAnySources && !showFetchPermissionButton &&
          getString(S.BRAVE_NEWS_DIRECT_SEARCH_NO_RESULTS).replace('$1', search.feedUrlQuery ?? '')
        }
      </DiscoverSection>
    </>
  )
}
