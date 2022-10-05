import * as React from 'react'
import styled from 'styled-components'
import { useChannelSubscribed, usePublisher, usePublisherSubscribed } from './Context'
import { useGetUnpaddedImage } from '../default/braveToday/cards/CardImage'
import Flex from '../Flex'
import { Heart, HeartOutline } from './Icons'

interface Props {
    publisherId: string
}

const Container = styled(Flex)`
    padding: 10px 0;
    cursor: pointer;
    
    :hover {
        opacity: 0.5;
    }
`

const FavIconContainer = styled.div`
    width: 24px;
    height: 24px;
    border-radius: 100px;

    img {
        width: 100%;
        height: 100%;
    }
`
const ToggleButton = styled.button`
    all: unset;
    cursor: pointer;
    color: var(--interactive5);
`

const Text = styled.span`
    font-size: 14px;
    font-weight: 500;
`

const ChannelNameText = styled.span`
    font-size: 14px;
    font-weight: 600;
`

function FavIcon (props: { src?: string }) {
    const url = useGetUnpaddedImage(props.src)
    const [error, setError] = React.useState(false)
    return <FavIconContainer>
        {url && !error && <img src={url} onError={() => setError(true)} />}
    </FavIconContainer>
}

export function FeedListEntry (props: Props) {
    const publisher = usePublisher(props.publisherId)
    const { subscribed, setSubscribed } = usePublisherSubscribed(props.publisherId)

    return <Container direction="row" justify="space-between" align='center' onClick={() => setSubscribed(!subscribed)}>
        <Flex align='center' gap={8}>
            <FavIcon src={publisher.faviconUrl?.url} />
            <Text>{publisher.publisherName}</Text>
        </Flex>
        <ToggleButton>
            {subscribed ? Heart : HeartOutline}
        </ToggleButton>
    </Container>
}

export function ChannelListEntry (props: { channelId: string }) {
    const { subscribed, setSubscribed } = useChannelSubscribed(props.channelId)

    return <Container direction="row" justify='space-between' align='center' onClick={() => setSubscribed(!subscribed)}>
        <ChannelNameText>{props.channelId}</ChannelNameText>
        <ToggleButton>
            {subscribed ? Heart : HeartOutline}
        </ToggleButton>
    </Container>
}
