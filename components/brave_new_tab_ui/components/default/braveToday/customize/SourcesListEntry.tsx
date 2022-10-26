// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'
import Flex from '../../../Flex'
import { useChannelSubscribed, usePublisher, usePublisherFollowed } from './Context'
import { useFavicon } from '../useUnpaddedImageUrl'
import { getTranslatedChannelName } from './ChannelCard'

interface Props {
  publisherId: string
}

const ToggleButton = styled.button`
  all: unset;
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
  width: 24px;
  height: 24px;
  flex-shrink: 0;
  border-radius: 100px;

  img {
    width: 100%;
    height: 100%;
  }
`

const Text = styled.span`
  font-size: 14px;
  font-weight: 500;
`

const ChannelNameText = styled.span`
  font-size: 14px;
  font-weight: 600;
`

function FavIcon (props: { publisherId: string }) {
  const url = useFavicon(props.publisherId)
  const [error, setError] = React.useState(false)
  return <FavIconContainer>
    {url && !error && <img src={url} onError={() => setError(true)} />}
  </FavIconContainer>
}

export function FeedListEntry (props: Props) {
  const publisher = usePublisher(props.publisherId)
  const { setFollowed } = usePublisherFollowed(props.publisherId)

  return <Container direction="row" justify="space-between" align='center'>
    <Flex align='center' gap={8}>
      <FavIcon publisherId={props.publisherId} />
      <Text>{publisher.publisherName}</Text>
    </Flex>
    <ToggleButton onClick={() => setFollowed(false)}>
      {getLocale('braveNewsFollowButtonFollowing')}
    </ToggleButton>
  </Container>
}

export function ChannelListEntry (props: { channelName: string }) {
  const { setSubscribed } = useChannelSubscribed(props.channelName)

  return <Container direction="row" justify='space-between' align='center'>
    <ChannelNameText>{getTranslatedChannelName(props.channelName)}</ChannelNameText>
    <ToggleButton onClick={() => setSubscribed(false)}>
      {getLocale('braveNewsFollowButtonFollowing')}
    </ToggleButton>
  </Container>
}
