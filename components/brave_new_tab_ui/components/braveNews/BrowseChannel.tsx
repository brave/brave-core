import Button from '$web-components/button'
import * as React from 'react'
import styled from 'styled-components'
import { useChannelSubscribed, usePublishers } from '../../api/brave_news/news'
import { useBraveNews } from './Context'
import FeedCard from './FeedCard'
import FollowButton from './FollowButton'
import { BackArrow } from './Icons'

const Container = styled.div`
    height: 100%;
    overflow: auto;
`

const BackButton = styled(Button)`
    justify-self: start;
`

const Header = styled.div`
    display: grid;
    grid-template-columns: min-content auto 120px;
    grid-template-rows: auto;
    justify-self: center;
    align-items: stretch;
    justify-items: center;
    margin-bottom: 36px;
    max-width: 660px;
`

const HeaderText = styled.span`
    font-weight: 500;
    font-size: 16px;
    align-self: center;
`

const FeedCardsContainer = styled('div')`
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 208px));
    gap: 40px 16px;
    margin-top: 12px;
`

export default function BrowseChannel (props: { channelId: string }) {
    const { setPage } = useBraveNews()
    const publishers = usePublishers({ channelId: props.channelId })
    const { subscribed, setSubscribed } = useChannelSubscribed(props.channelId)

    return <Container>
        <Header>
            <BackButton onClick={() => setPage('news')}>
                {BackArrow} Back
            </BackButton>
            <HeaderText>
                {props.channelId}
            </HeaderText>
            <FollowButton following={subscribed} onClick={() => setSubscribed(!subscribed)}/>
        </Header>
        <FeedCardsContainer>
            {publishers.map((p) => <FeedCard
                key={p.publisherId}
                publisherId={p.publisherId}/>)}
        </FeedCardsContainer>
    </Container>
}
