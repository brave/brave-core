// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from '$web-common/Flex'
import * as React from 'react'
import styled from 'styled-components'
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import { useChannelSubscribed } from '../shared/Context'
import { channelIcons } from '../shared/Icons'
import FollowButton from '../shared/FollowButton'
import { getTranslatedChannelName } from '../shared/channel'

const SubscribeButton = styled(FollowButton)`
  position: absolute;
  top: ${spacing.m};
  right: ${spacing.m};
`

const Container = styled(Flex)`
  height: 80px;
  font: ${font.default.semibold};
  color: ${color.text.primary};
  border-radius: ${radius.m};
  padding: ${spacing.xl};
  position: relative;
  box-shadow: ${effect.elevation['02']};
  border: 1px solid ${color.divider.subtle};
  background: ${color.container.background};

  &[data-channel-card-is-followed=true] {
    &:not(:hover, :has(:focus-visible)) ${SubscribeButton} {
      opacity: 0;
    }
  }
`

const IconContainer = styled.div`
  --leo-icon-size: ${spacing.xl};

  width: ${spacing['3Xl']};
  height: ${spacing['3Xl']};
  padding: ${spacing.m};
  border-radius: ${radius.full};
  background: ${color.container.highlight};
  color: ${color.icon.default};
  display: flex;
  align-items: center;
  justify-content: center;
`

interface Props {
  channelName: string
}

export default function ChannelCard({ channelName }: Props) {
  const { subscribed, setSubscribed } = useChannelSubscribed(channelName)
  return <Container
    direction='column'
    justify='center'
    align='start'
    gap={spacing.s}
    data-channel-card-is-followed={subscribed}
  >
    <SubscribeButton following={subscribed} onClick={() => setSubscribed(!subscribed)} />
    <IconContainer>
      {channelIcons[channelName] ?? channelIcons.default}
    </IconContainer>
    {getTranslatedChannelName(channelName)}
  </Container>
}
