// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { useState } from 'react'
import * as React from 'react'
import styled from 'styled-components'
import { api } from '../../../../api/brave_news/news'
import Flex from '../../../Flex'
import FollowButton from './FollowButton'
import { getCardColor } from './colors'
import { usePublisher, usePublisherFollowed } from './Context'
import { useLazyUnpaddedImageUrl } from '../useUnpaddedImageUrl'

interface CardProps {
  backgroundColor?: string
}

const StyledFollowButton = styled(FollowButton)`
  position: absolute;
  right: 8px;
  top: 8px;
`

const Card = styled('div').attrs<CardProps>(props => ({
  style: {
    backgroundColor: props.backgroundColor
  }
}))<CardProps>`
  position: relative;
  height: 80px;
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0px 0px 16px 0px #63696E2E;

  &[data-feed-card-is-followed=true] {
    &:not(:hover, :has(:focus-visible)) ${StyledFollowButton} {
      opacity: 0;
    }
  }
`

const CoverImage = styled('div') <{ backgroundImage: string }>`
  position: absolute;
  top: 15%; bottom: 15%; left: 15%; right: 15%;
  border-radius: 8px;
  background-position: center;
  background-size: contain;
  background-repeat: no-repeat;
  background-image: url('${p => p.backgroundImage}');
`

const Name = styled.span`
  font-size: 14px;
  font-weight: 600;
`

export default function FeedCard (props: {
  publisherId: string
}) {
  const publisher = usePublisher(props.publisherId)
  const { followed, setFollowed } = usePublisherFollowed(props.publisherId)

  const backgroundColor = publisher.backgroundColor || getCardColor(publisher.feedSource?.url || publisher.publisherId)
  const { url: coverUrl, elementRef } = useLazyUnpaddedImageUrl(publisher.coverUrl?.url, {
    rootElement: document.getElementById('brave-news-configure'),
    rootMargin: '0px 0px 200px 0px',
    useCache: true
  })

  return <Flex direction="column" gap={8} ref={elementRef}>
    <Card backgroundColor={backgroundColor} data-feed-card-is-followed={followed}>
      {coverUrl && <CoverImage backgroundImage={coverUrl} />}
      <StyledFollowButton following={followed} onClick={() => setFollowed(!followed)} />
    </Card>
    <Name>
      {publisher.publisherName}
    </Name>
  </Flex>
}

export function DirectFeedCard (props: {
  feedUrl: string
  title: string
}) {
  const [loading, setLoading] = useState(false)
  return <Flex direction="column" gap={8}>
    <Card backgroundColor={getCardColor(props.feedUrl)} data-feed-card-is-followed={true}>
      <StyledFollowButton isDisabled={loading} following={false} onClick={async () => {
        setLoading(true)
        await api.subscribeToDirectFeed(props.feedUrl)
        setLoading(false)
      }} />
    </Card>
    <Name>
      {props.title}
    </Name>
  </Flex>
}
