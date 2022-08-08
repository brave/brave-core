
import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'
import { useChannels, usePublishers } from '../../api/brave_news/news'
import Flex from '../Flex'
import { formatMessage } from '../../../brave_rewards/resources/shared/lib/locale_context'

import { FeedListEntry, ChannelListEntry } from './SourcesListEntry'

const Title = styled.span`
  font-size: 18px;
  font-weight: 800;
  line-height: 36px;
`

const Subtitle = styled.span`
  font-weight: 500;
  font-size: 12px;
  color: #868e96;
`

export default function SourcesList () {
    const publishers = usePublishers({ subscribed: true })
    const channels = useChannels({ subscribedOnly: true })

    return <div>
        <Flex direction="row" justify="space-between" align="center">
            <Title>{getLocale('braveNewsFeedsHeading')}</Title>
            <Subtitle>{formatMessage(getLocale('braveNewsSourceCount'), [publishers.length + channels.length])}</Subtitle>
        </Flex>
        <Flex direction="column">
            {channels.map(c => <ChannelListEntry key={c.channelName} channelId={c.channelName} />)}
            {publishers.map((p) => (
                <FeedListEntry key={p.publisherId} publisherId={p.publisherId} />
            ))}
        </Flex>
    </div>
}
