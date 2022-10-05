import Button from '$web-components/button'
import TextInput from '$web-components/input'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'
import { useChannels, usePublishers } from '../../api/brave_news/news'
import Flex from '../Flex'
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

const LoadMoreButton = styled(Button)`
    grid-column: 2;
`

// The default number of category cards to show.
const DEFAULT_NUM_CATEGORIES = 3

export default function Discover (props: {}) {
    const channels = useChannels()
    const [showingAllCategories, setShowingAllCategories] = React.useState(false)
    const [query, setQuery] = useState('')
    const publishers = usePublishers()

    const { filteredChannels, filteredPublishers } = React.useMemo(() => {
        const lowerQuery = query.toLowerCase()
        return {
            filteredChannels: channels.filter(c => c.channelName.toLowerCase().includes(lowerQuery)),
            filteredPublishers: publishers.filter(p => p.publisherName.toLowerCase().includes(lowerQuery) ||
                p.categoryName.toLocaleLowerCase().includes(lowerQuery) ||
                p.feedSource?.url?.toLocaleLowerCase().includes(query))
        }
    }, [query, publishers, channels])

    return <Flex direction='column'>
        <Header>Discover</Header>
        <SearchInput type="search" placeholder={getLocale('braveNewsSearchPlaceholderLabel')} value={query} onChange={e => setQuery(e.currentTarget.value)} />
        <DirectFeedResults query={query} />
        <DiscoverSection name={getLocale('braveNewsBrowseByCategoryHeader')}>
            {filteredChannels
                // If we're showing all channels, there's no end to the slice.
                // Otherwise, just show the default number.
                .slice(0, showingAllCategories || query
                    ? undefined
                    : DEFAULT_NUM_CATEGORIES)
                .filter(t => t.channelName.toLowerCase().includes(query.toLowerCase()))
                .map(c => <ChannelCard key={c.channelName} channelId={c.channelName} />)}
            {!showingAllCategories && !query &&
                <LoadMoreButton onClick={() => setShowingAllCategories(true)}>
                    {getLocale('braveNewsLoadMoreCategoriesButton')}
                </LoadMoreButton>}
        </DiscoverSection>
        <DiscoverSection name={getLocale('braveNewsAllSourcesHeader')}>
            {filteredPublishers.map(p => <FeedCard key={p.publisherId} publisherId={p.publisherId} />)}
        </DiscoverSection>
    </Flex>
}
