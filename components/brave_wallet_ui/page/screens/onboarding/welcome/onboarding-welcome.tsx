// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useReportOnboardingActionMutation //
} from '../../../../common/slices/api.slice'

// components
import {
  WalletPageLayout //
} from '../../../../components/desktop/wallet-page-layout/index'
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button'

// routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// styles
import {
  Row,
  VerticalSpace,
  WalletWelcomeGraphic
} from '../../../../components/shared/style'
import { OnboardingWrapper } from '../onboarding.style'
import {
  Title,
  ButtonContainer,
  LearnMoreLink,
  BlockQuote,
  BlockQuoteTextContainer,
  VerticalRule,
  SubDivider,
  SubDividerText
} from './onboarding-welcome.style'

export const OnboardingWelcome = () => {
  // routing
  const history = useHistory()

  // mutations
  const [report] = useReportOnboardingActionMutation()

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.Shown)
  }, [report])

  return (
    <WalletPageLayout>
      <OnboardingWrapper>
        <WalletWelcomeGraphic />

        <Title maxWidth='467px'>{getLocale('braveWalletWelcomeTitle')}</Title>

        <BlockQuote>
          <VerticalRule />
          <BlockQuoteTextContainer>
            <span>{getLocale('braveWalletPerksTokens')}</span>
            <span>{getLocale('braveWalletMultiChain')}</span>
            <span>{getLocale('braveWalletPerksBrowserNative')}</span>
          </BlockQuoteTextContainer>
        </BlockQuote>

        <VerticalSpace space='34px' />

        <ButtonContainer>
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletWelcomeButton')}
            onSubmit={() => history.push(WalletRoutes.OnboardingNewWalletTerms)}
            maxHeight={'48px'}
            minWidth={'267px'}
          />

          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletImportExistingWallet')}
            onSubmit={() => history.push(WalletRoutes.OnboardingImportTerms)}
            maxHeight={'48px'}
            minWidth={'267px'}
          />
        </ButtonContainer>

        <Row>
          <SubDivider />
          <SubDividerText>
            {getLocale('braveWalletWelcomeDividerText')}
          </SubDividerText>
          <SubDivider />
        </Row>

        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletConnectHardwareWallet')}
          onSubmit={() =>
            history.push(WalletRoutes.OnboardingHardwareWalletTerms)
          }
          maxHeight={'48px'}
          minWidth={'267px'}
        />

        <VerticalSpace space='20px' />

        <LearnMoreLink
          href='https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
          target='_blank'
          rel='noreferrer'
        >
          {getLocale('braveWalletLearnMoreAboutBraveWallet')}
        </LearnMoreLink>
      </OnboardingWrapper>
    </WalletPageLayout>
  )
}
