// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useLocation } from 'react-router'
import * as leo from '@brave/leo/tokens/css/variables'

// utils
import { getLocale } from '../../../../../common/locale'
import { getWordIndicesToVerify } from '../../../../utils/ordinal-utils'
import {
  useCompleteWalletBackupMutation,
  useReportOnboardingActionMutation
} from '../../../../common/slices/api.slice'

// selectors
import {
  useSafePageSelector //
} from '../../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../selectors'

// routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import Button from '@brave/leo/react/button'
import {
  OnboardingContentLayout //
} from '../../onboarding/components/onboarding_content_layout/content_layout'
import { PhraseInput } from './components/phrase_input'
import { VerificationProgress } from './components/verification_progress'

// styles
import {
  Title //
} from '../../onboarding/components/onboarding_content_layout/content_layout.style'
import { Column, Row } from '../../../../components/shared/style'
import { BackButton } from './verify_recovery_phrase.style'
import { ContinueButton } from '../../onboarding/onboarding.style'

const numberOfVerificationSteps = 3

export const VerifyRecoveryPhrase = () => {
  // state
  const [currentStep, setCurrentStep] = React.useState(0)
  const [enteredPhrase, setEnteredPhrase] = React.useState('')
  const [showError, setShowError] = React.useState(false)

  // mutations
  const [report] = useReportOnboardingActionMutation()

  // redux
  const dispatch = useDispatch()
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)

  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // mutations
  const [completeWalletBackup] = useCompleteWalletBackupMutation()

  // methods
  const onSkipBackup = React.useCallback(() => {
    dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
    if (isOnboarding) {
      report(BraveWallet.OnboardingAction.CompleteRecoverySkipped)
      history.push(WalletRoutes.OnboardingComplete)
      return
    }
    history.push(WalletRoutes.PortfolioAssets)
  }, [dispatch, history, isOnboarding, report])

  const onNextStep = React.useCallback(async () => {
    await completeWalletBackup().unwrap()
    if (isOnboarding) {
      dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
      history.push(WalletRoutes.OnboardingComplete)
      return
    }
    history.push(WalletRoutes.PortfolioAssets)
  }, [completeWalletBackup, isOnboarding, history, dispatch])

  const onContinue = () => {
    if (showError) setShowError(false)
    // check if the word matches
    if (enteredPhrase === verificationWord) {
      setEnteredPhrase('')

      if (currentStep === numberOfVerificationSteps - 1) {
        onNextStep()
      } else {
        setCurrentStep(currentStep + 1)
      }
    } else {
      setShowError(true)
    }
  }

  const onChangePhrase = (value: string) => {
    // reset error state
    if (showError) setShowError(false)

    setEnteredPhrase(value)
  }

  // memos
  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  const verificationIndices = React.useMemo(
    () => getWordIndicesToVerify(recoveryPhrase.length),
    [recoveryPhrase.length]
  )

  const verificationIndex = verificationIndices[currentStep]
  const verificationWord = recoveryPhrase[verificationIndex]
  const wordPosition = verificationIndex + 1

  return (
    <OnboardingContentLayout
      title={
        <Row
          width='100%'
          gap='24px'
          justifyContent='center'
          alignItems='center'
        >
          <Title>{getLocale('braveWalletVerifyRecoveryPhraseTitle')}</Title>
          <VerificationProgress
            steps={numberOfVerificationSteps}
            currentStep={currentStep}
          />
        </Row>
      }
      showBackButton={false}
    >
      <PhraseInput
        phrase={enteredPhrase}
        showError={showError}
        wordPosition={wordPosition}
        onHideError={() => setShowError(false)}
        onChange={onChangePhrase}
      />
      <BackButton
        onClick={() =>
          history.push(WalletRoutes.OnboardingBackupRecoveryPhrase)
        }
      >
        {getLocale('braveWalletVerifyRecoveryPhraseGoBack')}
      </BackButton>

      <Column
        margin='100px 0 0 0'
        gap='24px'
      >
        <ContinueButton
          isDisabled={enteredPhrase === ''}
          onClick={onContinue}
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
        <Button
          kind='plain-faint'
          color={leo.color.text.secondary}
          onClick={onSkipBackup}
        >
          {getLocale('braveWalletButtonSkip')}
        </Button>
      </Column>
    </OnboardingContentLayout>
  )
}
