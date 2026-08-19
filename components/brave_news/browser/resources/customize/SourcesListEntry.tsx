// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import { color, effect, font, icon, radius, spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import { useChannelSubscribed, usePublisher, usePublisherFollowed } from '../shared/Context'
import { channelIcons as ChannelIcons } from '../shared/Icons'
import { getTranslatedChannelName } from '../shared/channel'

interface Props {
  publisherId: string
}

const RemoveButton = styled.button`
  all: unset;
  --leo-icon-size: ${icon.m};

  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  color: ${color.icon.default};
  padding: ${spacing.s};
  border-radius: ${radius.s};

  &:hover {
    color: ${color.icon.interactive};
  }

  &:focus-visible {
    outline: ${effect.focusState};
  }
`

const Container = styled(Flex)`
  padding: ${spacing.m} 0;
  min-width: 0;

  &:not(:hover, :has(:focus-visible)) ${RemoveButton} {
    opacity: 0;
  }
`

const FavIconContainer = styled.div`
  flex: 0 0 ${icon.l};
  height: ${icon.l};
  flex-shrink: 0;
  border-radius: ${radius.full};
  color: ${color.icon.default};

  img {
    width: 100%;
    height: 100%;
  }
`

const Text = styled.span`
  flex: 1 1 0;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font: ${font.default.regular};
  color: ${color.text.primary};
`

function FavIcon (props: { publisherId: string }) {
  const publisher = usePublisher(props.publisherId)
  const faviconUrl = publisher.faviconUrl?.url
  const [error, setError] = React.useState(false)

  React.useEffect(() => {
    setError(false)
  }, [faviconUrl])

  return (
    <FavIconContainer>
      {faviconUrl && !error && <img loading='lazy' src={`//brave-image?url=${encodeURIComponent(faviconUrl)}`} onError={() => setError(true)} />}
    </FavIconContainer>
  )
}

export function FeedListEntry (props: Props) {
  const publisher = usePublisher(props.publisherId)
  const { setFollowed } = usePublisherFollowed(props.publisherId)
  const unfollowLabel = getLocale(S.BRAVE_NEWS_FOLLOW_BUTTON_FOLLOWING)

  return (
    <Container direction="row" justify="space-between" align='center' gap={spacing.m}>
      <FavIcon publisherId={props.publisherId} />
      <Text title={publisher.publisherName}>{publisher.publisherName}</Text>
      <RemoveButton
        onClick={() => setFollowed(false)}
        title={unfollowLabel}
        aria-label={unfollowLabel}
      >
        <Icon name='trash' />
      </RemoveButton>
    </Container>
  )
}

export function ChannelListEntry (props: { channelName: string }) {
  const { setSubscribed } = useChannelSubscribed(props.channelName)
  const channelName = getTranslatedChannelName(props.channelName)
  const unfollowLabel = getLocale(S.BRAVE_NEWS_FOLLOW_BUTTON_FOLLOWING)

  return (
    <Container direction="row" justify='space-between' align='center' gap={spacing.m}>
      <FavIconContainer>
        {ChannelIcons[props.channelName] ?? ChannelIcons.default}
      </FavIconContainer>
      <Text title={channelName}>{channelName}</Text>
      <RemoveButton
        onClick={() => setSubscribed(false)}
        title={unfollowLabel}
        aria-label={unfollowLabel}
      >
        <Icon name='trash' />
      </RemoveButton>
    </Container>
  )
}
