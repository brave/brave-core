import { useState, useEffect } from 'react'
import * as React from 'react'
import styled, { keyframes } from 'styled-components'
import Flex from '../Flex'
import FollowButton from './FollowButton'
import { Heart, HeartOutline } from './Icons'
import { api, usePublisher, usePublisherSubscribed } from '../../api/brave_news/news'
import { getCardColor } from './colors'
import { useGetUnpaddedImage } from '../default/braveToday/cards/CardImage'

const Container = styled(Flex)`
`

const Card = styled('div') <{ backgroundColor?: string }>`
    position: relative;
    height: 80px;
    background-color: ${p => p.backgroundColor};
    border-radius: 8px;
    overflow: hidden;
    box-shadow: 0px 0px 16px 0px #63696E2E;
`

const CoverImage = styled('div')<{ backgroundImage: string }>`
    position: absolute;
    top: 15%; bottom: 15%; left: 15%; right: 15%;
    border-radius: 8px;
    background-position: center;
    background-size: contain;
    background-repeat: no-repeat;
    background-image: url('${p => p.backgroundImage}');
`

const StyledFollowButton = styled(FollowButton)`
    position: absolute;
    right: 8px;
    top: 8px;
`

const Name = styled.span`
    font-size: 14px;
    font-weight: 600;
`

const Pulse = keyframes`
    0% {
        pointer-events: auto;
    }
    5% { opacity: 1; }
    80% { opacity: 1; }
    99% {
        pointer-events: auto;
    }
    100% { 
        pointer-events: none;
        opacity: 0;
    }
`

const HeartOverlay = styled(Flex)`
    pointer-events: none;
    background: white;
    color: #aeb1c2;
    position: absolute;
    top: 0;
    bottom: 0;
    left: 0;
    right: 0;
    opacity: 0;
    animation: ${Pulse} 2s ease-in-out;
`

const HeartContainer = styled.div`
    width: 32px;
    height: 32px;

    > svg {
        width: 100%;
        height: 100%;
    }
`

export default function FeedCard (props: {
    publisherId: string
}) {
    const publisher = usePublisher(props.publisherId)
    const { subscribed, setSubscribed } = usePublisherSubscribed(props.publisherId)
    const [changeCount, setToggleCount] = useState(0)
    useEffect(() => {
        setToggleCount(t => t + 1)
    }, [subscribed])

    const backgroundColor = publisher.backgroundColor || getCardColor(publisher.feedSource?.url || publisher.publisherId)
    const coverUrl = useGetUnpaddedImage(publisher.coverUrl?.url ?? '', false)
    return <Container direction="column" gap={8}>
        <Card backgroundColor={backgroundColor}>
            {coverUrl && <CoverImage backgroundImage={coverUrl} />}
            <StyledFollowButton following={subscribed} onClick={() => setSubscribed(!subscribed)} />

            {/*
                Use whether or not we're following this element as the key, so
                React remounts the component when we toggle following and plays
                the animation.

                We don't display the overlay unless we've toggled this publisher
                so we don't play the pulse animation on first load.
            */}
            {changeCount > 1 && <HeartOverlay key={changeCount} align="center" justify="center">
                <HeartContainer>
                    {subscribed ? Heart : HeartOutline}
                </HeartContainer>
            </HeartOverlay>}
        </Card>
        <Name>
            {publisher.publisherName}
        </Name>
    </Container>
}

export function DirectFeedCard (props: {
    feedUrl: string
    title: string
}) {
    const [loading, setLoading] = useState(false)
    return <Container direction="column" gap={8}>
        <Card backgroundColor={getCardColor(props.feedUrl)}>
            <StyledFollowButton isDisabled={loading} following={false} onClick={async () => {
                setLoading(true)
                await api.subscribeToDirectFeed(props.feedUrl)
                setLoading(false)
            }} />
        </Card>
        <Name>
            {props.title}
        </Name>
    </Container>
}
