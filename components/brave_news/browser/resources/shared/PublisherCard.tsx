// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import * as React from 'react'
import { useState } from 'react'
import styled from 'styled-components'
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { getCardColor } from '../customize/colors'
import { usePublisher, usePublisherFollowed } from './Context'
import FollowButton from './FollowButton'
import getBraveNewsController from './api'

interface CardProps {
  backgroundColor?: string
}

const StyledFollowButton = styled(FollowButton)`
  position: absolute;
  right: ${spacing.m};
  top: ${spacing.m};
`

const Card = styled('div').attrs<CardProps>(props => ({
  style: {
    backgroundColor: props.backgroundColor
  }
})) <CardProps>`
  position: relative;
  height: 80px;
  border-radius: ${radius.m};
  overflow: hidden;
  box-shadow: ${effect.elevation['02']};

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
})) <CoverImageProps>`
  position: absolute;
  top: 15%; bottom: 15%; left: 15%; right: 15%;
  border-radius: ${radius.m};
  background-position: center;
  background-size: contain;
  background-repeat: no-repeat;
`

const Name = styled.span`
  font: ${font.default.semibold};
  color: ${color.text.primary};
`

export default function PublisherCard(props: {
  publisherId: string
}) {
  const publisher = usePublisher(props.publisherId)
  const { followed, setFollowed } = usePublisherFollowed(props.publisherId)

  const backgroundColor = publisher?.backgroundColor || getCardColor(publisher?.feedSource?.url || publisher?.publisherId)
  const coverUrl = publisher?.coverUrl?.url

  return <Flex direction="column" gap={spacing.m}>
    <Card backgroundColor={backgroundColor} data-feed-card-is-followed={followed}>
      {coverUrl && <CoverImage backgroundImage={`//brave-image?url=${encodeURIComponent(coverUrl)}`} />}
      <StyledFollowButton fab size='tiny' following={followed} onClick={() => setFollowed(!followed)} />
    </Card>
    <Name>
      {publisher?.publisherName}
    </Name>
  </Flex>
}

export function DirectPublisherCard(props: {
  feedUrl: string
  title: string
}) {
  const [loading, setLoading] = useState(false)
  return <Flex direction="column" gap={spacing.m}>
    <Card backgroundColor={getCardColor(props.feedUrl)} data-feed-card-is-followed={true}>
      <StyledFollowButton following={false} isDisabled={loading} onClick={async () => {
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
