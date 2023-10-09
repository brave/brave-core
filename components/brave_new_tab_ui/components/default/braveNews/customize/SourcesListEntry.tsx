// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import Flex from '$web-common/Flex'
import { useLazyFavicon } from '../../../../../brave_news/browser/resources/shared/useUnpaddedImageUrl'
import { useChannelSubscribed, usePublisher, usePublisherFollowed } from '../../../../../brave_news/browser/resources/shared/Context'
import { getTranslatedChannelName } from './ChannelCard'
import { channelIcons as ChannelIcons } from '../../../../../brave_news/browser/resources/shared/Icons'

interface Props {
  publisherId: string
}

const ToggleButton = styled.button`
  all: unset;
  flex: 0 0 auto;
  cursor: pointer;
  color: var(--brave-color-text02);
  &:hover {
    text-decoration: underline;
  }
  &:active {
    color: var(--brave-color-interactive08);
  }
  &:focus-visible {
    outline: 1px solid var(--brave-color-focusBorder);
    outline-offset: 4px;
  }
`

const Container = styled(Flex)`
  padding: 10px 0;

  &:not(:hover, :has(:focus-visible)) ${ToggleButton} {
    opacity: 0;
  }
`

const FavIconContainer = styled.div`
  flex: 0 0 24px;
  height: 24px;
  flex-shrink: 0;
  border-radius: 100px;
  color: #6B7084;

  img {
    width: 100%;
    height: 100%;
  }
`

const Text = styled.span`
  flex: 1 1 0;
  word-break: break-word;
  font-size: 14px;
  font-weight: 500;
`

const ChannelNameText = styled(Text)`
  font-weight: 600;
`

function FavIcon (props: { publisherId: string }) {
  const { url, setElementRef } = useLazyFavicon(props.publisherId, {
    rootElement: document.getElementById('brave-news-configure'),
    rootMargin: '200px 0px 200px 0px'
  })
  const [error, setError] = React.useState(false)
  return (
    <FavIconContainer ref={setElementRef}>
      {url && !error && <img src={url} onError={() => setError(true)} />}
    </FavIconContainer>
  )
}

export function FeedListEntry (props: Props) {
  const publisher = usePublisher(props.publisherId)
  const { setFollowed } = usePublisherFollowed(props.publisherId)

  return (
    <Container direction="row" justify="space-between" align='center' gap={8}>
      <FavIcon publisherId={props.publisherId} />
      <Text>{publisher.publisherName}</Text>
      <ToggleButton onClick={() => setFollowed(false)}>
        {getLocale('braveNewsFollowButtonFollowing')}
      </ToggleButton>
    </Container>
  )
}

export function ChannelListEntry (props: { channelName: string }) {
  const { setSubscribed } = useChannelSubscribed(props.channelName)

  return (
    <Container direction="row" justify='space-between' align='center' gap={8}>
      <FavIconContainer>
        {ChannelIcons[props.channelName] ?? ChannelIcons.default}
      </FavIconContainer>
      <ChannelNameText>{getTranslatedChannelName(props.channelName)}</ChannelNameText>
      <ToggleButton onClick={() => setSubscribed(false)}>
        {getLocale('braveNewsFollowButtonFollowing')}
      </ToggleButton>
    </Container>
  )
}
