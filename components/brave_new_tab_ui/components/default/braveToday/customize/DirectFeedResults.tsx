import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { useDirectFeedResults } from './Context'
import DiscoverSection from './DiscoverSection'
import { DirectFeedCard } from './FeedCard'

interface Props {
    query: string
}

export default function SearchResults (props: Props) {
    const { loading, directResults } = useDirectFeedResults(props.query)

    return <div>
        {loading
            ? <span>{getLocale('braveNewsSearchResultsLoading')}</span>
            : <>
                {!!directResults.length &&
                    <DiscoverSection name={getLocale('braveNewsSearchResultsDirectResults')}>
                        {directResults.map(r => <DirectFeedCard key={r.feedUrl.url} feedUrl={r.feedUrl.url} title={r.feedTitle} />)}
                    </DiscoverSection>}
            </>}
    </div>
}
