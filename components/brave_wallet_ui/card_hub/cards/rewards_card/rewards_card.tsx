// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Constants
import { LOCAL_STORAGE_KEYS } from '$wallet/common/constants/local-storage-keys'

// Utils
import { getLocale } from '$web-common/locale'
import { useSyncedLocalStorage } from '$wallet/common/hooks/use_local_storage'

// Components
import { Card } from '../card'

// Styles
import {
  RewardsCardBackground,
  BraveIcon,
  IconWrapper,
  VerifyTextWrapper,
  BalanceText,
  DescriptionText,
} from './rewards_card.style'
import { Column, Row, Text } from '$wallet/components/shared/style'

interface Props {
  onClick?: () => void
}

export const RewardsCard = (props: Props) => {
  const { onClick } = props

  // Local Storage
  const [hidePortfolioBalances] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES,
    false,
  )

  return (
    <Card onClick={onClick}>
      <RewardsCardBackground />
      <Column
        justifyContent='space-between'
        alignItems='flex-start'
        padding='16px'
        width='100%'
        height='100%'
        gap='8px'
      >
        <Row
          gap='8px'
          justifyContent='space-between'
          alignItems='center'
        >
          <IconWrapper>
            <BraveIcon name='brave-icon-release-color' />
          </IconWrapper>
          <Column
            justifyContent='center'
            alignItems='flex-end'
          >
            <BalanceText
              variant='xSmall.regular'
              textColor='white'
            >
              {getLocale(S.BRAVE_WALLET_BALANCE)}
            </BalanceText>
            <Text
              variant='heading.h3'
              textColor='white'
            >
              {hidePortfolioBalances ? '$••••' : '$0.00'}
            </Text>
          </Column>
        </Row>
        <Column
          justifyContent='flex-start'
          alignItems='flex-start'
          gap='6px'
        >
          <Text
            variant='heading.h3'
            textColor='white'
          >
            {getLocale(S.BRAVE_WALLET_GET_YOUR_CARD)}
          </Text>
          <DescriptionText
            variant='small.regular'
            textColor='white'
            textAlign='left'
          >
            {getLocale(S.BRAVE_WALLET_GET_YOUR_CARD_DESCRIPTION)}
          </DescriptionText>
          <VerifyTextWrapper>
            <Text
              variant='small.regular'
              textColor='white'
            >
              {getLocale(S.BRAVE_WALLET_TAP_TO_VERIFY)}
            </Text>
          </VerifyTextWrapper>
        </Column>
      </Column>
    </Card>
  )
}
