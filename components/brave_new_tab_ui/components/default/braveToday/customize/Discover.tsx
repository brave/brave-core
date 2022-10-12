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

const Hidable = styled.div<{ hidden: boolean }>`
  display: ${p => p.hidden ? 'none' : 'unset'};
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
  const { sortedPublishers, filteredPublisherIds } = useBraveNews()

  const visibleChannelIds = React.useMemo(() => new Set(channels
    // If we're showing all channels, there's no end to the slice.
    // Otherwise, just show the default number.
    .slice(0, showingAllCategories
      ? undefined
      : DEFAULT_NUM_CATEGORIES)
    .map(c => c.channelName)),
    [channels, showingAllCategories])

  return (
    <>
      <DiscoverSection name={getLocale('braveNewsChannelsHeader')}>
      {channels.map(c => <Hidable key={c.channelName} hidden={!visibleChannelIds.has(c.channelName)}>
        <ChannelCard key={c.channelName} channelId={c.channelName} />
      </Hidable>)}
      {!showingAllCategories && <LoadMoreButtonContainer>
        <Button onClick={() => setShowingAllCategories(true)}>
            {getLocale('braveNewsLoadMoreCategoriesButton')}
        </Button>
      </LoadMoreButtonContainer>}
      </DiscoverSection>
      <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
      {sortedPublishers.map(p =>
        <Hidable key={p.publisherId} hidden={!filteredPublisherIds.includes(p.publisherId)}>
          <FeedCard publisherId={p.publisherId} />
        </Hidable>)}
      </DiscoverSection>
    </>
  )
}

interface SearchResultsProps {
  query: string
}
function SearchResults (props: SearchResultsProps) {
  const { sortedPublishers } = useBraveNews()
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
        {sortedPublishers.map(r =>
          <Hidable key={r.publisherId} hidden={!search.filteredSources.publishers.includes(r.publisherId)}>
            <FeedCard publisherId={r.publisherId} />
          </Hidable>
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
