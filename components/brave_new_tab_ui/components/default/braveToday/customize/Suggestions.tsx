import * as React from 'react'
import styled from 'styled-components'
import Button from '$web-components/button'
import { useBraveNews } from './Context'
import DiscoverSection from './DiscoverSection'
import FeedCard from './FeedCard'
import { getLocale } from '$web-common/locale'

const LoadMoreButtonContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: stretch;
  grid-column: 2;
`

const Subtitle = styled.span`
  font-weight: 400;
  font-size: 12px;
  color: var(--text2);
`

const DEFAULT_SUGGESTIONS_COUNT = 3

export default function Suggestions () {
  const { suggestedPublisherIds } = useBraveNews()
  const [showAll, setShowAll] = React.useState(false)
  const filteredSuggestions = React.useMemo(() => suggestedPublisherIds
    .slice(0, showAll ? undefined : DEFAULT_SUGGESTIONS_COUNT), [suggestedPublisherIds, showAll])

  return filteredSuggestions.length
    ? <DiscoverSection name={getLocale('braveNewsSuggestionsTitle')} subtitle={<>
      <Subtitle>{getLocale('braveNewsSuggestionsSubtitle')}</Subtitle>
    </>}>
      {filteredSuggestions.map(s => <FeedCard key={s} publisherId={s} />)}
      {!showAll && suggestedPublisherIds.length > DEFAULT_SUGGESTIONS_COUNT && <LoadMoreButtonContainer>
        <Button onClick={() => setShowAll(true)}>
          {getLocale('braveNewsShowMoreButton')}
        </Button>
      </LoadMoreButtonContainer>}
    </DiscoverSection>
    : null
}
