// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

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
  BannerText
} from './explain-recovery-phrase.style'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'

// images
import ExamplePhrase from './images/example-recovery-phrase.svg'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

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
  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingBackupRecoveryPhrase)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Onboarding)
  }, [])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={WalletRoutes.OnboardingExplainRecoveryPhrase}
          />

          <div>
            <Title>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}</Title>
            <Description>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroDescription')}</Description>
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
              onSubmit={nextStep}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingRecoveryPhraseExplainer
