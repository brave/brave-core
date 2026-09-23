// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { Crypto } from './cards/crypto/crypto'
import { Rewards } from './cards/rewards/rewards'
import { RewardsCard } from './cards/rewards_card/rewards_card'

// Styles
import { CardStack, WalletLogo, Wrapper } from './card_hub.style'
import { Column, Row } from '$wallet/components/shared/style'

export const CardHub = () => {
  return (
    <Wrapper
      justifyContent='flex-start'
      alignItems='center'
      width='100%'
      height='100%'
    >
      <Row padding='24px'>
        <WalletLogo />
      </Row>
      <Column
        width='100%'
        padding='16px'
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
