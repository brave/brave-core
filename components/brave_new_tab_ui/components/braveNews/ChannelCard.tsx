import styled, { css } from 'styled-components'
import * as React from 'react'
import Flex from '../Flex'
import { useBraveNews } from './Context'
import { Channel } from '../../api/brave_news'
import { getCardColor } from './colors'

const Container = styled(Flex) <{ height: number, backgroundColor: string, backgroundUrl?: string }>`
    height: ${p => `${p.height}px`};
    font-weight: 600;
    font-size: 14px;
    border-radius: 8px;
    background: ${p => p.backgroundColor};
    ${p => p.backgroundUrl && css`
        background-image: url("${p.backgroundUrl}");
    `}
    padding: 16px 20px;
    color: white;

    :hover {
        opacity: 0.8;
    }
`

interface Props {
    channel: Channel &
        // TODO: Include these in the channel.
        { icon?: any, backgroundColor?: string, backgroundUrl?: string }
}

export default function ChannelCard ({ channel }: Props) {
    const { setPage } = useBraveNews()

    return <Container
        onClick={() => setPage(`channel/${channel.channelName}`)}
        direction='column'
        justify={channel.icon ? 'end' : 'center'}
        align={channel.icon ? 'start' : 'center'}
        backgroundColor={channel.backgroundColor ?? getCardColor(channel.channelName)}
        backgroundUrl={channel.backgroundUrl}
        height={channel.icon ? 102 : 80}
    >
        {channel.icon}
        {channel.channelName}
    </Container>
}
