// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as React from 'react'
import Flex from '../../../Flex'
import { getCardColor } from './colors'
import FollowButton from './FollowButton'
import { useChannelSubscribed } from './Context'

const SubscribeButton = styled(FollowButton)`
    position: absolute;
    top: 8px;
    right: 8px;
`

const Container = styled(Flex) <{ backgroundColor: string }>`
  height: 80px;
  font-weight: 600;
  font-size: 14px;
  border-radius: 8px;
  background: ${p => p.backgroundColor};
  padding: 16px 20px;
  color: white;
  position: relative;

  &[data-channel-card-is-followed=true] {
    &:not(:hover, :has(:focus-visible)) ${SubscribeButton} {
      opacity: 0;
    }
  }
`

interface Props {
  channelName: string
}

export default function ChannelCard ({ channelName }: Props) {
  const { subscribed, setSubscribed } = useChannelSubscribed(channelName)
  return <Container
    direction='column'
    justify='center'
    align='center'
    backgroundColor={getCardColor(channelName)}
    data-channel-card-is-followed={subscribed}
  >
    <SubscribeButton following={subscribed} onClick={() => setSubscribed(!subscribed)} />
    {channelName}
  </Container>
}
