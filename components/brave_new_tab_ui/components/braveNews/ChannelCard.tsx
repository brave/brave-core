import styled from 'styled-components'
import * as React from 'react'
import Flex from '../Flex'
import { getCardColor } from './colors'
import FollowButton from './FollowButton'
import { useChannelSubscribed } from '../../api/brave_news/news'

const Container = styled(Flex) <{ backgroundColor: string }>`
    height: 80px;
    font-weight: 600;
    font-size: 14px;
    border-radius: 8px;
    background: ${p => p.backgroundColor};
    padding: 16px 20px;
    color: white;
    position: relative;

    :hover {
        opacity: 0.8;
    }
`

const SubscribeButton = styled(FollowButton)`
    position: absolute;
    top: 8px;
    right: 8px;
`

interface Props {
    channelId: string
}

export default function ChannelCard ({ channelId }: Props) {
    const { subscribed, setSubscribed } = useChannelSubscribed(channelId)
    return <Container
        direction='column'
        justify='center'
        align='center'
        backgroundColor={getCardColor(channelId)}
    >
        <SubscribeButton following={subscribed} onClick={() => setSubscribed(!subscribed)} />
        {channelId}
    </Container>
}
