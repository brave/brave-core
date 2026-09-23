// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import { Card } from '../card'

// Styles
import {
  RewardsBackground,
  BATIcon,
  EnableRewardsWrapper,
  EnableRewardsText,
} from './rewards.style'
import { Column, Row, Text } from '$wallet/components/shared/style'

interface Props {
  onClick?: () => void
}

export const Rewards = (props: Props) => {
  const { onClick } = props

  return (
    <Card onClick={onClick}>
      <RewardsBackground />
      <Column
        justifyContent='flex-start'
        alignItems='flex-start'
        padding='8px 16px'
        width='100%'
        gap='8px'
      >
        <Row
          gap='8px'
          justifyContent='flex-start'
          alignItems='center'
        >
          <BATIcon name='product-bat-color' />
          <Text
            variant='default.semibold'
            textColor='white'
          >
            {getLocale(S.BRAVE_WALLET_BRAVE_REWARDS_TITLE)}
          </Text>
        </Row>
        <Row
          padding='0px 0px 0px 28px'
          justifyContent='space-between'
          alignItems='center'
          gap='8px'
        >
          <Text
            variant='heading.h3'
            textColor='white'
          >
            0.00 BAT
          </Text>
          <EnableRewardsWrapper>
            <EnableRewardsText variant='small.semibold'>
              {getLocale(S.BRAVE_WALLET_ENABLE_REWARDS)}
            </EnableRewardsText>
          </EnableRewardsWrapper>
        </Row>
      </Column>
    </Card>
  )
}
