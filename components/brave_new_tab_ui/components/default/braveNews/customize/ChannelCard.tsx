// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as React from 'react'
import Flex from '../../../Flex'
import FollowButton from './FollowButton'
import { useChannelSubscribed } from './Context'
import { channels } from './Icons'
import { getLocale } from '$web-common/locale'

export const getTranslatedChannelName = (channelName: string) => {
  try {
    return getLocale(`braveNewsChannel-${channelName}`)
  } catch (err) {
    console.error(`Couldn't find translation for channel '${channelName}'`)
    return channelName
  }
}

const SubscribeButton = styled(FollowButton)`
    position: absolute;
    top: 8px;
    right: 8px;
`

const Container = styled(Flex)`
  height: 80px;
  font-weight: 600;
  font-size: 14px;
  border-radius: 8px;
  padding: 16px 20px;
  position: relative;
  box-shadow: 0px 2px 8px -1px rgba(0, 0, 0, 0.08), 0px 0.4px 1.5px rgba(0, 0, 0, 0.02);
  border: 1px solid rgba(0, 0, 0, 0.08);

  @media (prefers-color-scheme: dark) {
    border: 1px solid rgba(255, 255, 255, 0.08);
  }

  &[data-channel-card-is-followed=true] {
    &:not(:hover, :has(:focus-visible)) ${SubscribeButton} {
      opacity: 0;
    }
  }
`

const IconContainer = styled.div`
  width: 32px;
  height: 32px;
  padding: 8px;
  border-radius: 100px;
  background: rgba(0,0,0,0.2);
  color: #6B7084;
  display: flex;
  align-items: center;
  justify-content: center;
`

interface Props {
  channelName: string
}

export default function ChannelCard ({ channelName }: Props) {
  const { subscribed, setSubscribed } = useChannelSubscribed(channelName)
  const icon = channels[channelName] ?? channels.default
  return <Container
    direction='column'
    justify='center'
    align='start'
    gap={4}
    data-channel-card-is-followed={subscribed}
  >
    <SubscribeButton following={subscribed} onClick={() => setSubscribed(!subscribed)} />
    <IconContainer>{icon}</IconContainer>
    {getTranslatedChannelName(channelName)}
  </Container>
}
