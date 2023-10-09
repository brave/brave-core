// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useState } from 'react'
import * as React from 'react'
import styled from 'styled-components'
import Flex from '$web-common/Flex'
import FollowButton from './FollowButton'
import { getCardColor } from './colors'
import { usePublisher, usePublisherFollowed } from '../../../../../brave_news/browser/resources/shared/Context'
import { useLazyUnpaddedImageUrl } from '../../../../../brave_news/browser/resources/shared/useUnpaddedImageUrl'
import getBraveNewsController from '../../../../../brave_news/browser/resources/shared/api'

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

interface CoverImageProps {
  backgroundImage: string
}

const CoverImage = styled('div').attrs<CoverImageProps>(props => ({
  style: {
    backgroundImage: `url('${props.backgroundImage}')`
  }
}))<CoverImageProps>`
  position: absolute;
  top: 15%; bottom: 15%; left: 15%; right: 15%;
  border-radius: 8px;
  background-position: center;
  background-size: contain;
  background-repeat: no-repeat;
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

  const backgroundColor = publisher?.backgroundColor || getCardColor(publisher?.feedSource?.url || publisher?.publisherId)
  const { url: coverUrl, setElementRef } = useLazyUnpaddedImageUrl(publisher?.coverUrl?.url, {
    rootElement: document.getElementById('brave-news-configure'),
    rootMargin: '0px 0px 200px 0px',
    useCache: true
  })

  return <Flex direction="column" gap={8} ref={setElementRef}>
    <Card backgroundColor={backgroundColor} data-feed-card-is-followed={followed}>
      {coverUrl && <CoverImage backgroundImage={coverUrl} />}
      <StyledFollowButton following={followed} onClick={() => setFollowed(!followed)} />
    </Card>
    <Name>
      {publisher?.publisherName}
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
        await getBraveNewsController().subscribeToNewDirectFeed({ url: props.feedUrl })
        setLoading(false)
      }} />
    </Card>
    <Name>
      {props.title}
    </Name>
  </Flex>
}
