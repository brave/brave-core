// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import {
  LinkText,
  PhraseCard,
  PhraseCardBottomRow,
  PhraseCardTopRow
} from './onboarding-backup-recovery-phrase.style'

import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer
} from '../onboarding.style'

// utils
import { getLocale } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { OnboardingSteps, OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { ToggleVisibilityButton } from '../../../../components/shared/style'

export const OnboardingRecoveryPhrase = () => {
  // routing
  const history = useHistory()

  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)

  // const revealPhrase = React.useCallback(() => {
  //   setIsPhraseShown(true)
  // }, [])

  const toggleShowPhrase = React.useCallback(() => {
    setIsPhraseShown(prev => !prev)
  }, [])

  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={OnboardingSteps.backupRecoveryPhrase}
          />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletRecoveryPhraseBackupTitle')}</Title>
            <Description>
              {getLocale('braveWalletRecoveryPhraseBackupWarning')}
              <LinkText>{getLocale('braveWalletWelcomePanelButton')}</LinkText>
            </Description>
          </TitleAndDescriptionContainer>

          <PhraseCard>
            <PhraseCardTopRow>
              <ToggleVisibilityButton
                isVisible={isPhraseShown}
                onClick={toggleShowPhrase}
              />
            </PhraseCardTopRow>
            <p>
              Example
            </p>
            <p>
              Example
            </p>
            <p>
              Example
            </p>
            <p>
              Example
            </p>
            <p>
              Example
            </p>

            <PhraseCardBottomRow>
              Bottom
            </PhraseCardBottomRow>
          </PhraseCard>

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

export default OnboardingRecoveryPhrase
