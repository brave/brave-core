// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale, getLocaleWithTags } from '../../../../../common/locale'

// routes
import { PageState, WalletRoutes } from '../../../../constants/types'

// styles
import { ErrorText, ErrorXIcon } from '../../../../components/shared/style'
import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard,
  PhraseCardBody
} from '../onboarding.style'
import {
  ErrorTextRow
} from './verify-backup-recovery-phrase.style'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { RecoveryPhrase } from '../components/recovery-phrase/recovery-phrase'
import {
  OnboardingSteps,
  OnboardingStepsNavigation
} from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

export const OnboardingVerifyRecoveryPhrase = () => {
  // routing
  const history = useHistory()

  // state
  const [nextStepEnabled, setNextStepEnabled] = React.useState(false)
  const [hasSelectedWords, setHasSelectedWords] = React.useState(false)

  // redux
  const { mnemonic } = useSelector(({ page }: { page: PageState }) => page)

  // methods
  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingComplete)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  const onSelectedWordsUpdated = React.useCallback((words: any[]) => {
    setHasSelectedWords(words.length === 3)
  }, [])

  const onPhraseVerificationUpdated = React.useCallback((doesWordOrderMatch: boolean) => {
    setNextStepEnabled(doesWordOrderMatch)
  }, [])

  // memos
  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={OnboardingSteps.verifyRecoveryPhrase}
          />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletVerifyRecoveryPhraseTitle')}</Title>
            <Description>
              <span>
                {getLocaleWithTags('braveWalletVerifyRecoveryPhraseInstructions', 3).map((text, i) => {
                  return <span key={text.duringTag || i}>
                    {text.beforeTag}
                    <b>{text.duringTag}</b>
                    {text.afterTag}
                  </span>
                })}
              </span>
            </Description>
          </TitleAndDescriptionContainer>

          <PhraseCard>
            <PhraseCardBody>
              <RecoveryPhrase
                verificationModeEnabled={true}
                hidden={false}
                recoveryPhrase={recoveryPhrase}
                onVerifyUpdate={onPhraseVerificationUpdated}
                onSelectedWordListChange={onSelectedWordsUpdated}
              />
            </PhraseCardBody>
          </PhraseCard>

          <ErrorTextRow hasError={!nextStepEnabled && hasSelectedWords}>
            {!nextStepEnabled && hasSelectedWords &&
              <>
                <ErrorXIcon />
                <ErrorText>
                  {getLocale('braveWalletVerifyPhraseError')}
                </ErrorText>
              </>
            }
          </ErrorTextRow>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              onSubmit={nextStep}
              disabled={!nextStepEnabled}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingVerifyRecoveryPhrase
