// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'
import { clearClipboard } from '../../../../utils/copy-to-clipboard'

// actions
import { WalletPageActions } from '../../../actions'

// types
import { PageState, WalletRoutes } from '../../../../constants/types'

// styles
import { ToggleVisibilityButton, WalletLink } from '../../../../components/shared/style'
import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow,
  PhraseCardTopRow,
  VerticalSpace
} from '../onboarding.style'
import { RecoveryTextArea, RecoveryTextInput } from './restore-from-recovery-phrase.style'

// components
import { NewPasswordInput, NewPasswordValues } from '../../../../components/shared/password-input/new-password-input'
import { WalletPageLayout } from '../../../../components/desktop'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'
import { NavButton } from '../../../../components/extension'

enum RestoreFromPhraseSteps {
  phrase = 'phrase',
  password = 'password',
  complete = 'complete'
}

const RESTORE_FROM_PHRASE_STEPS: RestoreFromPhraseSteps[] = [
  RestoreFromPhraseSteps.phrase,
  RestoreFromPhraseSteps.password,
  RestoreFromPhraseSteps.complete
]

export const OnboardingRestoreFromRecoveryPhrase = () => {
  // routing
  let history = useHistory()

  // redux
  const dispatch = useDispatch()
  const invalidMnemonic = useSelector(({ page }: { page: PageState }) => page.invalidMnemonic)
  // const isWalletCreated = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletCreated)
  // const isWalletLocked = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletLocked)

  // state
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [phraseInput, setPhraseInput] = React.useState('')
  const [currentStep, setCurrentStep] = React.useState<RestoreFromPhraseSteps>(
    RestoreFromPhraseSteps.phrase
  )

  // methods
  const toggleShowPhrase = React.useCallback(() => {
    setIsPhraseShown(prev => !prev)
  }, [])

  const onClickPasteFromClipboard = React.useCallback(async () => {
    const phraseFromClipboard = await navigator.clipboard.readText()
    setPhraseInput(phraseFromClipboard.trim())
    clearClipboard()
  }, [])

  const restoreWallet = React.useCallback(async () => {
    if (invalidMnemonic || !isPasswordValid) {
      return
    }

    dispatch(WalletPageActions.restoreWallet({
      // added an additional trim here in case the phrase length is
      // 12, 15, 18 or 21 long and has a space at the end.
      mnemonic: phraseInput.trimEnd(),
      password,
      isLegacy: false,
      completeWalletSetup: false // postpone until wallet onboarding success screen
    }))

    history.push(WalletRoutes.OnboardingComplete)
  }, [phraseInput, password, invalidMnemonic, isPasswordValid])

  const onPhraseInputChanged = React.useCallback((event: React.ChangeEvent<
    HTMLInputElement | HTMLTextAreaElement
  >) => {
    const value = event.target.value

    // This prevents there from being a space at the begining of the phrase.
    const removeBegginingWhiteSpace = value.trimStart()

    // This Prevents there from being more than one space between words.
    const removedDoubleSpaces = removeBegginingWhiteSpace.replace(/ +(?= )/g, '')

    // Although the above removes double spaces, it is initialy recognized as a
    // a double-space before it is removed and macOS automatically replaces double-spaces with a period.
    const removePeriod = removedDoubleSpaces.replace(/['/.']/g, '')

    // This prevents an extra space at the end of a 24 word phrase.
    const cleanedInput = phraseInput.split(' ').length === 24
      ? removePeriod.trimEnd()
      : removePeriod

    setPhraseInput(cleanedInput)

    // isValid
    dispatch(WalletPageActions.hasMnemonicError(
      cleanedInput.trim().split(/\s+/g).length < 12
    ))
  }, [phraseInput])

  const handlePasswordChange = React.useCallback(({ isValid, password }: NewPasswordValues) => {
    setPassword(password)
    setIsPasswordValid(isValid)
  }, [])

  const onContinueClicked = React.useCallback(() => {
    if (currentStep === RestoreFromPhraseSteps.phrase && !invalidMnemonic) {
      return setCurrentStep(RestoreFromPhraseSteps.password)
    }
    if (currentStep === RestoreFromPhraseSteps.password && isPasswordValid) {
      return restoreWallet()
    }
  }, [currentStep, invalidMnemonic, isPasswordValid, restoreWallet])

  const onGoBack = React.useCallback(() => {
    if (currentStep === RestoreFromPhraseSteps.phrase) {
      return history.goBack()
    }
    if (currentStep === RestoreFromPhraseSteps.password) {
      return setCurrentStep(RestoreFromPhraseSteps.phrase)
    }
  }, [currentStep])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLElement>) => {
    if (event.key === 'Enter') {
      onContinueClicked()
    }
  }, [onContinueClicked, invalidMnemonic])

  // memos
  const RecoveryInput = React.useMemo(() => {
    return isPhraseShown
    ? <RecoveryTextArea
        as='textarea'
        value={phraseInput}
        onChange={onPhraseInputChanged}
        onPaste={clearClipboard}
        onKeyDown={handleKeyDown}
      >
        {phraseInput}
      </RecoveryTextArea>
    : <RecoveryTextInput
        type='password'
        value={phraseInput}
        onChange={onPhraseInputChanged}
        onPaste={clearClipboard}
        onKeyDown={handleKeyDown}
      />
  }, [isPhraseShown, phraseInput, onPhraseInputChanged, handleKeyDown])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            steps={RESTORE_FROM_PHRASE_STEPS}
            currentStep={currentStep}
            goBack={onGoBack}
            preventSkipAhead
          />

          {
            <TitleAndDescriptionContainer>
              <Title>
                {currentStep === RestoreFromPhraseSteps.phrase && getLocale('braveWalletRecoveryPhraseBackupTitle')}
                {currentStep === RestoreFromPhraseSteps.password && getLocale('braveWalletCreatePasswordTitle')}
              </Title>
              <Description>
                {currentStep === RestoreFromPhraseSteps.phrase && getLocale('braveWalletRecoveryPhraseBackupWarning')}
                {currentStep === RestoreFromPhraseSteps.password && getLocale('braveWalletCreatePasswordDescription')}
              </Description>
            </TitleAndDescriptionContainer>
          }

          {currentStep === RestoreFromPhraseSteps.phrase &&
            <>
              <PhraseCard>
                <PhraseCardTopRow>
                  <ToggleVisibilityButton
                    isVisible={isPhraseShown}
                    onClick={toggleShowPhrase}
                  />
                </PhraseCardTopRow>

                <PhraseCardBody>
                  {RecoveryInput}
                </PhraseCardBody>

                <PhraseCardBottomRow centered>
                  <WalletLink
                    as='button'
                    onClick={onClickPasteFromClipboard}
                  >
                    {getLocale('braveWalletPasteFromClipboard')}
                  </WalletLink>
                </PhraseCardBottomRow>
              </PhraseCard>

              <VerticalSpace space={isPhraseShown ? '20px' : '130px'} />
            </>
          }

          {currentStep === RestoreFromPhraseSteps.password &&
            <NewPasswordInput
              autoFocus={true}
              onSubmit={restoreWallet}
              onChange={handlePasswordChange}
            />
          }

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonContinue')}
              onSubmit={onContinueClicked}
              disabled={
                !phraseInput ||
                currentStep === RestoreFromPhraseSteps.phrase && invalidMnemonic ||
                currentStep === RestoreFromPhraseSteps.password && !isPasswordValid
              }
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}
