// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Components
import { Crypto } from './cards/crypto/crypto'
import { Rewards } from './cards/rewards/rewards'
import { RewardsCard } from './cards/rewards_card/rewards_card'
import { HubMenu } from './components/hub_menu/hub_menu'

// Styles
import { CardStack, FabButton, WalletLogo, Wrapper } from './card_hub.style'
import { Column, Row } from '$wallet/components/shared/style'

export const CardHub = () => {
  // Methods
  const onOpenNotifications = () => {
    // Will nav to notifications in the future
    alert('Open notifications')
  }

  const onOpenSettings = () => {
    // Will nav to settings in the future
    alert('Open settings')
  }

  return (
    <Wrapper
      justifyContent='flex-start'
      alignItems='center'
      width='100%'
      height='100%'
    >
      <Row
        padding='24px'
        justifyContent='space-between'
        alignItems='center'
        width='100%'
      >
        <FabButton onClick={onOpenNotifications}>
          <Icon name='notification-dot' />
        </FabButton>
        <WalletLogo />
        <HubMenu onOpenSettings={onOpenSettings} />
      </Row>
      <Column
        width='100%'
        padding='0 16px 16px 16px'
      >
        <CardStack>
          <Crypto />
          <Rewards />
          <RewardsCard />
        </CardStack>
      </Column>
    </Wrapper>
  )
}
