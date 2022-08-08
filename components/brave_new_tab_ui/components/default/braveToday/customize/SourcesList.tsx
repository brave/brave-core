// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import { useChannels, usePublishers } from './Context'
import Flex from '../../../Flex'
import { FeedListEntry, ChannelListEntry } from './SourcesListEntry'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '../../../../hooks/usePromise'

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

  const { result: sourcesCount } = usePromise(async () => PluralStringProxyImpl.getInstance().getPluralString('braveNewsSourceCount', publishers.length + channels.length), [publishers.length, channels.length])

  return <div>
    <Flex direction="row" justify="space-between" align="center">
      <Title>{getLocale('braveNewsFeedsHeading')}</Title>
      <Subtitle>{sourcesCount}</Subtitle>
    </Flex>
    <Flex direction="column">
      {channels.map(c => <ChannelListEntry key={c.channelName} channelId={c.channelName} />)}
      {publishers.map((p) => (
        <FeedListEntry key={p.publisherId} publisherId={p.publisherId} />
      ))}
    </Flex>
  </div>
}
