// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'

// images
import ExamplePhrase from './images/example-recovery-phrase.svg'

// components
import { NavButton } from '../../../../components/extension'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { ArticleLinkBubble } from '../onboarding-success/components/article-link-bubble/article-link-bubble'

// style
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper
} from '../onboarding.style'
import {
  BannerCard,
  WarningCircle,
  ImportantText,
  BannerText,
  CenteredRow
} from './explain-recovery-phrase.style'

const importantTextParts = splitStringForTag(getLocale('braveWalletRecoveryPhraseBackupWarningImportant'))

const ImportantTextSegments = () => {
  return <BannerText>
    {importantTextParts.beforeTag}
    <ImportantText>{importantTextParts.duringTag}</ImportantText>
    {importantTextParts.afterTag}
  </BannerText>
}

export const OnboardingRecoveryPhraseExplainer = () => {
  // routing
  const history = useHistory()

  // methods
  const skipToOnboardingSuccess = () => {
    history.push(WalletRoutes.OnboardingComplete)
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            preventGoBack
            currentStep={WalletRoutes.OnboardingExplainRecoveryPhrase}
            onSkip={skipToOnboardingSuccess}
          />

          <div>
            <Title>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}</Title>
            <Description>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroDescription')}</Description>
            <CenteredRow>
              <ArticleLinkBubble
                icon='key'
                iconBackgroundColor='red200'
                text={getLocale('braveWalletArticleLinkWhatsARecoveryPhrase')}
                url='https://brave.com/learn/wallet-recovery-phrase/'
              />
            </CenteredRow>
          </div>

          <img width='376px' height='118px' src={ExamplePhrase} />

          <BannerCard>
            <WarningCircle />
            <ImportantTextSegments />
          </BannerCard>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonGotIt')}
              url={WalletRoutes.OnboardingBackupRecoveryPhrase}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default OnboardingRecoveryPhraseExplainer
