// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// actions
import { WalletPageActions } from '../../../actions'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import {
  OnboardingDisclosures,
  OnboardingDisclosuresNextSteps
} from '../disclosures/disclosures'

// routes
import { PageState, WalletRoutes } from '../../../../constants/types'

// styles
import { VerticalSpace, WalletWelcomeGraphic } from '../../../../components/shared/style'
import { OnboardingWrapper } from '../onboarding.style'
import {
  Title,
  ButtonContainer,
  LearnMoreLink,
  BlockQuote,
  BlockQuoteTextContainer,
  VerticalRule
} from './onboarding-welcome.style'

export const OnboardingWelcome = () => {
  // redux
  const dispatch = useDispatch()
  const setupStillInProgress = useSelector(({ page }: { page: PageState }) => page.setupStillInProgress)

  // state
  const [nextStep, setNextStep] = React.useState<OnboardingDisclosuresNextSteps | undefined>(undefined)

  // methods
  const hideDisclosures = React.useCallback(() => setNextStep(undefined), [])
  const showNewWalletDisclosures = React.useCallback(
    () => setNextStep(WalletRoutes.OnboardingCreatePassword),
    []
  )

  const showRestoredWalletDisclosures = React.useCallback(
    () => setNextStep(WalletRoutes.OnboardingImportOrRestore),
    []
  )

  // effects
  React.useEffect(() => {
    // start wallet setup
    if (!setupStillInProgress) {
      dispatch(WalletPageActions.walletSetupComplete(false))
    }
    dispatch(WalletPageActions.onOnboardingShown())
  }, [setupStillInProgress])

  // render
  if (nextStep !== undefined) {
    return <OnboardingDisclosures nextStep={nextStep} onBack={hideDisclosures} />
  }

  return <WalletPageLayout>
    <OnboardingWrapper>
      <WalletWelcomeGraphic />

      <Title maxWidth='467px'>
        {getLocale('braveWalletWelcomeTitle')}
      </Title>

      <BlockQuote>
        <VerticalRule />
        <BlockQuoteTextContainer>
          <span>
            {getLocale('braveWalletPerksTokens')}
          </span>
          <span>
            {getLocale('braveWalletPerksNftStorage')}
          </span>
          <span>
            {getLocale('braveWalletMultiChain')}
          </span>
        </BlockQuoteTextContainer>
      </BlockQuote>

      <VerticalSpace space='34px' />

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletWelcomeButton')}
          onSubmit={showNewWalletDisclosures}
          maxHeight={'48px'}
          minWidth={'267px'}
        />

        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletWelcomeIAlreadyHaveAWallet')}
          onSubmit={showRestoredWalletDisclosures}
          maxHeight={'48px'}
          minWidth={'267px'}
        />

      </ButtonContainer>

      <VerticalSpace space='20px' />

      <LearnMoreLink
        href='https://brave.com/learn/what-is-crypto-wallet/'
        target='_blank'
        rel='noreferrer'
      >
        {getLocale('braveWalletWhatIsACryptoWallet')}
      </LearnMoreLink>

    </OnboardingWrapper>

  </WalletPageLayout>
}
