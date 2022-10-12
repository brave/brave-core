import Button from '$web-components/button'
import TextInput from '$web-components/input'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import { useChannels, usePublishers } from './Context'
import Flex from '../../../Flex'
import ChannelCard from './ChannelCard'
import DiscoverSection from './DiscoverSection'
import FeedCard from './FeedCard'
import DirectFeedResults from './DirectFeedResults'

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

const Hidable = styled.div<{ hidden: boolean }>`
  display: ${p => p.hidden ? 'none' : 'unset'};
`

// The default number of category cards to show.
const DEFAULT_NUM_CATEGORIES = 3

export default function Discover () {
  const channels = useChannels()
  const [showingAllCategories, setShowingAllCategories] = React.useState(false)
  const [query, setQuery] = useState('')
  const publishers = usePublishers()

  const lowerQuery = query.toLowerCase()
  const visiblePublisherIds = React.useMemo(() => new Set(publishers
    .filter(p => p.publisherName.toLowerCase().includes(lowerQuery) ||
      p.categoryName.toLocaleLowerCase().includes(lowerQuery) ||
      p.feedSource?.url?.toLocaleLowerCase().includes(query))
      .map(p => p.publisherId))
    ,
    [lowerQuery, publishers])

  const visibleChannelIds = React.useMemo(() => new Set(channels
    .filter(c => c.channelName.toLowerCase().includes(lowerQuery))
    // If we're showing all channels, there's no end to the slice.
    // Otherwise, just show the default number.
    .slice(0, showingAllCategories || query
      ? undefined
      : DEFAULT_NUM_CATEGORIES)
    .map(c => c.channelName)),
    [lowerQuery, channels, showingAllCategories])

  return <Flex direction='column'>
    <Header>Discover</Header>
    <SearchInput type="search" placeholder={getLocale('braveNewsSearchPlaceholderLabel')} value={query} onChange={e => setQuery(e.currentTarget.value)} />
    <DirectFeedResults query={query} />
    <DiscoverSection name={getLocale('braveNewsBrowseByCategoryHeader')}>
      {channels.map(c => <Hidable key={c.channelName} hidden={!visibleChannelIds.has(c.channelName)}>
        <ChannelCard key={c.channelName} channelId={c.channelName} />
      </Hidable>)}
      {!showingAllCategories && !query && <LoadMoreButtonContainer>
        <Button onClick={() => setShowingAllCategories(true)}>
          {getLocale('braveNewsLoadMoreCategoriesButton')}
        </Button>
      </LoadMoreButtonContainer>}
    </DiscoverSection>
    <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
      {publishers.map(p => <Hidable key={p.publisherId} hidden={!visiblePublisherIds.has(p.publisherId)}>
        <FeedCard publisherId={p.publisherId} />
      </Hidable>)}
    </DiscoverSection>
  </Flex>
}
