// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Constants
import { WalletRoutes } from '../../../../../../constants/types'

// Hooks
import { useRoute } from '../../../../../../common/hooks/use_route'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Styled components
import {
  GettingStartedWrapper,
  Title,
  CardsWrapper,
  Card,
  CardTitle,
  CardDescription,
} from './getting_started.style'
import { Column } from '../../../../../shared/style'

export const GettingStarted = () => {
  // Routing
  const { openOrPushRoute } = useRoute()

  // Methods
  const onClickBuyCrypto = () => {
    openOrPushRoute(WalletRoutes.FundWalletPageStart)
  }

  const onClickDepositCrypto = () => {
    openOrPushRoute(WalletRoutes.DepositFundsPageStart)
  }

  return (
    <GettingStartedWrapper
      fullWidth={true}
      gap='32px'
    >
      <Title>{getLocale('braveWalletLetsGetStarted')}</Title>
      <CardsWrapper gap='16px'>
        <Card onClick={onClickBuyCrypto}>
          <Icon name='coins-alt1' />
          <Column alignItems='flex-start'>
            <CardTitle textAlign='left'>
              {getLocale('braveWalletBuyCryptoButton')}
            </CardTitle>
            <CardDescription
              textColor='tertiary'
              textAlign='left'
            >
              {getLocale('braveWalletLetsGetStartedBuyCryptoDescription')}
            </CardDescription>
          </Column>
        </Card>
        <Card onClick={onClickDepositCrypto}>
          <Icon name='qr-code-alternative' />
          <Column alignItems='flex-start'>
            <CardTitle textAlign='left'>
              {getLocale('braveWalletDepositFundsTitle')}
            </CardTitle>
            <CardDescription
              textColor='tertiary'
              textAlign='left'
            >
              {getLocale('braveWalletLetsGetStartedDepositCryptoDescription')}
            </CardDescription>
          </Column>
        </Card>
      </CardsWrapper>
    </GettingStartedWrapper>
  )
}
