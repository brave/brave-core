// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
// import { getLocale } from '../../../../../common/locale'

// images
import WalletAccessSvg from './images/wallet-access.svg'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'

// routes
import { WalletRoutes } from '../../../../constants/types'

// styles
import {
  Description,
  Title,
  OnboardingWrapper
} from '../onboarding.style'
import {
  ButtonContainer, IntroContainer, IntroImg
} from './onboarding-success.style'

export const OnboardingSuccess = () => {
  const history = useHistory()

  const onSetup = React.useCallback(() => {
    history.push(`${WalletRoutes.Onboarding}/create-password`)
  }, [])

  return <WalletPageLayout>
    <OnboardingWrapper>

      <IntroContainer>
        <Title>{
          // getLocale('braveWalletWelcomeTitle')
          'Congrats! Your wallet is ready'
        }</Title>

        <Description textAlign='center'>{
          // getLocale('braveWalletWelcomeDescription')
          'Now you can easily access your wallet any time from the wallet icon in Brave Browser.'
        }</Description>

        <IntroImg src={WalletAccessSvg} height={118} />

      </IntroContainer>

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={
            'Go To Portfolio' // TODO: locale
          }
          onSubmit={onSetup}
        />

      </ButtonContainer>

    </OnboardingWrapper>
  </WalletPageLayout>
}
