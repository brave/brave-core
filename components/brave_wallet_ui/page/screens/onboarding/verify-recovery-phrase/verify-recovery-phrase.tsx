// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale, getLocaleWithTags } from '../../../../../common/locale'

// routes
import { PageState, WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

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
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

export const OnboardingVerifyRecoveryPhrase = () => {
  // state
  const [nextStepEnabled, setNextStepEnabled] = React.useState(false)
  const [hasSelectedWords, setHasSelectedWords] = React.useState(false)

  // redux
  const dispatch = useDispatch()
  const { mnemonic } = useSelector(({ page }: { page: PageState }) => page)

  // routing
  const history = useHistory()

  // methods
  const onSelectedWordsUpdated = React.useCallback((words: any[]) => {
    setHasSelectedWords(words.length === 3)
  }, [])

  const onPhraseVerificationUpdated = React.useCallback((doesWordOrderMatch: boolean) => {
    setNextStepEnabled(doesWordOrderMatch)
  }, [])

  const onNextStep = React.useCallback(() => {
    dispatch(WalletPageActions.walletBackupComplete())
    history.push(WalletRoutes.OnboardingComplete)
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

          <OnboardingNewWalletStepsNavigation
            goBackUrl={WalletRoutes.OnboardingExplainRecoveryPhrase}
            currentStep={WalletRoutes.OnboardingVerifyRecoveryPhrase}
            onSkip={onNextStep}
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
              disabled={!nextStepEnabled}
              onSubmit={onNextStep}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingVerifyRecoveryPhrase
