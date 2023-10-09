// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button from '@brave/leo/react/button'
import TextInput from '$web-components/input'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import { useBraveNews, useChannels } from '../../../../../brave_news/browser/resources/shared/Context'
import Flex from '$web-common/Flex'
import ChannelCard from './ChannelCard'
import DiscoverSection from './DiscoverSection'
import FeedCard, { DirectFeedCard } from './FeedCard'
import useSearch from './useSearch'
import { SuggestionsCarousel } from './Suggestions'
import { PopularCarousel } from './Popular'

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
    <SearchInput type="search" placeholder={getLocale('braveNewsSearchPlaceholderLabel')} value={query} onChange={e => setQuery(e.currentTarget.value)} />
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
      <DiscoverSection name={getLocale('braveNewsChannelsHeader')}>
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
      <DiscoverSection name={getLocale('braveNewsChannelsHeader')}>
        {search.filteredChannels.map(c =>
          <ChannelCard key={c.channelName} channelName={c.channelName} />
        )}
      </DiscoverSection>
      }
      <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
        {search.filteredSources.publisherIds.map(publisherId =>
          <FeedCard key={publisherId} publisherId={publisherId} />
        )}
        {showFetchPermissionButton &&
          <div>
            <Button size='tiny' kind='plain-faint' onClick={() => search.setCanFetchUrl(true)} isLoading={search.loading}>
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
