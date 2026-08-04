/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import usePromise from '$web-common/usePromise'
import {
  color,
  font,
  spacing,
} from '@brave/leo/tokens/css/variables'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import * as React from 'react'
import styled from 'styled-components'
import { useBraveNews, useChannels } from '../shared/Context'
import Loading from './Loading'
import { ChannelListEntry, FeedListEntry } from './SourcesListEntry'

const Container = styled.div`
  display: flex;
  flex-direction: column;
  padding: 0 ${spacing.xl};
`

const Heading = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: ${spacing.m};
  padding: ${spacing.m};
`

const Title = styled.span`
  font: ${font.default.semibold};
  color: ${color.text.primary};
`

const Count = styled.span`
  font: ${font.small.regular};
  color: ${color.text.tertiary};
`

const List = styled.div`
  display: flex;
  flex-direction: column;
`

export default function SourcesList() {
  const { subscribedPublisherIds, publishersLoaded, channelsLoaded } = useBraveNews()
  const channels = useChannels({ subscribedOnly: true })
  const isLoading = !publishersLoaded || !channelsLoaded

  const { result: sourcesCount } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        S.BRAVE_NEWS_SOURCE_COUNT,
        subscribedPublisherIds.length + channels.length,
      ),
    [subscribedPublisherIds.length, channels.length],
  )

  return (
    <Container>
      <Heading>
        <Title>{getLocale(S.BRAVE_NEWS_FEEDS_HEADING)}</Title>
        {!isLoading && <Count>{sourcesCount}</Count>}
      </Heading>
      <List>
        {isLoading ? (
          <Loading />
        ) : (
          <>
            {channels.map((c) => (
              <ChannelListEntry key={c.channelName} channelName={c.channelName} />
            ))}
            {subscribedPublisherIds.map((p) => (
              <FeedListEntry key={p} publisherId={p} />
            ))}
          </>
        )}
      </List>
    </Container>
  )
}
