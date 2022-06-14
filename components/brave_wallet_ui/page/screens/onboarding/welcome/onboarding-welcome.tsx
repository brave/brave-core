// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// actions
import { WalletPageActions } from '../../../actions'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'

// routes
import { WalletRoutes } from '../../../../constants/types'

// styles
import { OnboardingWrapper } from '../onboarding.style'
import {
  PageIcon,
  Description,
  Title,
  ButtonContainer,
  LearnMoreLink
} from './onboarding-welcome.style'

export const OnboardingWelcome = () => {
  // redux
  const dispatch = useDispatch()

  // effects
  React.useEffect(() => {
    dispatch(WalletPageActions.walletSetupComplete(false))
  }, [])

  return <WalletPageLayout>
    <OnboardingWrapper>
      <PageIcon />

      <Title>{getLocale('braveWalletWelcomeTitle')}</Title>

      <Description>{getLocale('braveWalletWelcomeDescription')}</Description>

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletWelcomeButton')}
          url={WalletRoutes.OnboardingDisclosures}
        />

        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletWelcomeIAlreadyHaveAWallet')}
          url={WalletRoutes.OnboardingImportOrRestore}
        />

        <LearnMoreLink href='https://brave.com/learn/what-is-crypto-wallet/' target='_blank'>{getLocale('braveWalletWhatIsACryptoWallet')}</LearnMoreLink>
      </ButtonContainer>

    </OnboardingWrapper>
  </WalletPageLayout>
}
