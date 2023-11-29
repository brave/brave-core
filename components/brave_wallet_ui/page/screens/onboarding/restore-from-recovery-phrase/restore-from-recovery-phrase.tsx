// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// constants
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useReportOnboardingActionMutation,
  useRestoreWalletMutation
} from '../../../../common/slices/api.slice'

// styles
import {
  VerticalSpace,
  Row,
  ErrorText
} from '../../../../components/shared/style'
import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard
} from '../onboarding.style'

// components
import {
  NewPasswordInput,
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { RecoveryInput } from './recovery-input'
import { Checkbox } from '../../../../components/shared/checkbox/checkbox'
import { CreatingWallet } from '../creating_wallet/creating_wallet'
import {
  OnboardingStepsNavigation //
} from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

type RestoreWalletSteps = 'phrase' | 'password'

const VALID_PHRASE_LENGTHS = [12, 15, 18, 21, 24]

export const OnboardingRestoreFromRecoveryPhrase = () => {
  // routing
  const history = useHistory()

  // mutations
  const [restoreWalletFromSeed, { isLoading: isCreatingWallet }] =
    useRestoreWalletMutation()
  const [report] = useReportOnboardingActionMutation()

  // state
  const [hasInvalidSeedError, setHasInvalidSeedError] = React.useState(false)
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [phraseInput, setPhraseInput] = React.useState('')
  const [phraseWordsLength, setPhraseWordsLength] = React.useState(0)
  const [isImportingFromLegacySeed, setIsImportingFromLegacySeed] =
    React.useState(false)
  const [currentStep, setCurrentStep] =
    React.useState<RestoreWalletSteps>('phrase')

  const importWalletError = hasInvalidSeedError
    ? getLocale('braveWalletInvalidMnemonicError')
    : undefined

  const isCorrectPhraseLength = VALID_PHRASE_LENGTHS.includes(phraseWordsLength)

  // methods
  const restoreWallet = React.useCallback(async () => {
    if (!isPasswordValid) {
      return
    }

    const { invalidMnemonic } = await restoreWalletFromSeed({
      // added an additional trim here in case the phrase length is
      // 12, 15, 18 or 21 long and has a space at the end.
      mnemonic: phraseInput.trimEnd(),
      password,
      isLegacy: isImportingFromLegacySeed,
      // postpone until wallet onboarding success screen
      completeWalletSetup: false
    }).unwrap()

    setHasInvalidSeedError(invalidMnemonic)

    if (!invalidMnemonic) {
      history.push(WalletRoutes.OnboardingComplete)
    }
  }, [
    isPasswordValid,
    phraseInput,
    password,
    isImportingFromLegacySeed,
    restoreWalletFromSeed
  ])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsPasswordValid(isValid)
    },
    []
  )

  const onContinueClicked = React.useCallback(async () => {
    if (currentStep === 'phrase') {
      return setCurrentStep('password')
    }

    if (currentStep === 'password' && isPasswordValid) {
      return await restoreWallet()
    }
  }, [currentStep, isPasswordValid, restoreWallet])

  const handleKeyDown = React.useCallback(
    (event: React.KeyboardEvent<HTMLElement>) => {
      if (event.key === 'Enter') {
        onContinueClicked()
      }
    },
    [onContinueClicked]
  )

  const onRecoveryInputChanged = React.useCallback(
    ({
      value,
      phraseLength
    }: {
      value: string
      isValid: boolean
      phraseLength: number
    }): void => {
      setPhraseInput(value)
      setPhraseWordsLength(phraseLength)
    },
    []
  )

  // effects
  React.useEffect(() => {
    if (currentStep === 'password') {
      report(BraveWallet.OnboardingAction.LegalAndPassword)
    }
  }, [report, currentStep])

  // render
  if (isCreatingWallet) {
    return <CreatingWallet />
  }

  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <OnboardingStepsNavigation preventSkipAhead />

          <TitleAndDescriptionContainer>
            <Title>
              {getLocale(
                currentStep === 'password'
                  ? 'braveWalletCreatePasswordTitle'
                  : 'braveWalletRestoreMyBraveWallet'
              )}
            </Title>
            <Description>
              {getLocale(
                currentStep === 'password'
                  ? 'braveWalletCreatePasswordDescription'
                  : 'braveWalletRestoreMyBraveWalletInstructions'
              )}
            </Description>
          </TitleAndDescriptionContainer>

          {currentStep === 'phrase' && (
            <>
              <PhraseCard>
                <RecoveryInput
                  onChange={onRecoveryInputChanged}
                  onKeyDown={handleKeyDown}
                  onToggleShowPhrase={setIsPhraseShown}
                />
              </PhraseCard>

              <VerticalSpace space={isPhraseShown ? '20px' : '130px'} />

              <Row
                alignItems='center'
                style={{ minHeight: 53 }}
              >
                {phraseWordsLength === 24 && (
                  <Checkbox
                    isChecked={isImportingFromLegacySeed}
                    onChange={setIsImportingFromLegacySeed}
                    disabled={false}
                  >
                    <Description>
                      {getLocale('braveWalletRestoreLegacyCheckBox')}
                    </Description>
                  </Checkbox>
                )}

                {phraseInput &&
                  phraseWordsLength > 12 &&
                  !isCorrectPhraseLength && (
                    <ErrorText>
                      {getLocale('braveWalletRecoveryPhraseLengthError')}
                    </ErrorText>
                  )}
              </Row>
            </>
          )}

          {currentStep === 'password' && (
            <>
              <NewPasswordInput
                autoFocus={true}
                onSubmit={restoreWallet}
                onChange={handlePasswordChange}
              />
              {importWalletError && <ErrorText>{importWalletError}</ErrorText>}
            </>
          )}

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonContinue')}
              onSubmit={onContinueClicked}
              disabled={
                !phraseInput ||
                (currentStep === 'phrase' &&
                  (!phraseInput ||
                    phraseWordsLength < 12 ||
                    (phraseWordsLength > 12 && !isCorrectPhraseLength))) ||
                (currentStep === 'password' &&
                  (!isPasswordValid || hasInvalidSeedError))
              }
            />
          </NextButtonRow>
        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
