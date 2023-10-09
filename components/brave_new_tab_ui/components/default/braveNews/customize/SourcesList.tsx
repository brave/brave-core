// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import { useBraveNews, useChannels } from '../../../../../brave_news/browser/resources/shared/Context'
import Flex from '$web-common/Flex'
import { FeedListEntry, ChannelListEntry } from './SourcesListEntry'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '$web-common/usePromise'

const Title = styled.span`
  font-size: 18px;
  font-weight: 600;
  line-height: 36px;
`

const Subtitle = styled.span`
  font-weight: 500;
  font-size: 12px;
  color: #868e96;
`

export default function SourcesList () {
  const { subscribedPublisherIds } = useBraveNews()
  const channels = useChannels({ subscribedOnly: true })

  const { result: sourcesCount } = usePromise(async () => PluralStringProxyImpl.getInstance().getPluralString('braveNewsSourceCount', subscribedPublisherIds.length + channels.length), [subscribedPublisherIds.length, channels.length])

  return <div>
    <Flex direction="row" justify="space-between" align="center">
      <Title>{getLocale('braveNewsFeedsHeading')}</Title>
      <Subtitle>{sourcesCount}</Subtitle>
    </Flex>
    <Flex direction="column">
      {channels.map(c => <ChannelListEntry key={c.channelName} channelName={c.channelName} />)}
      {subscribedPublisherIds.map((p) => (
        <FeedListEntry key={p} publisherId={p} />
      ))}
    </Flex>
  </div>
}
