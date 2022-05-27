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
import { LoadingSkeleton, PasswordInput } from '../../../../components/shared'

enum RestoreFromOtherWalletSteps {
  phrase = 'phrase',
  newPassword = 'password',
  currentPassword = 'current-password',
  complete = 'complete'
}

const RESTORE_FROM_PHRASE_STEPS: RestoreFromOtherWalletSteps[] = [
  RestoreFromOtherWalletSteps.phrase,
  RestoreFromOtherWalletSteps.newPassword,
  RestoreFromOtherWalletSteps.complete
]

const RESTORE_FROM_METAMASK_STEPS: RestoreFromOtherWalletSteps[] = [
  RestoreFromOtherWalletSteps.currentPassword,
  RestoreFromOtherWalletSteps.newPassword,
  RestoreFromOtherWalletSteps.complete
]

interface Props {
  restoreFrom: 'metamask' | 'seed' | 'metamask-seed'
}

export const OnboardingRestoreFromRecoveryPhrase = ({
  restoreFrom = 'seed'
}: Props) => {
  // routing
  let history = useHistory()

  // redux
  const dispatch = useDispatch()
  const invalidMnemonic = useSelector(({ page }: { page: PageState }) => page.invalidMnemonic)
  const isMetaMaskInitialized = useSelector(({ page }: { page: PageState }) => page.isMetaMaskInitialized)
  const isImportWalletsCheckComplete = useSelector(({ page }: { page: PageState }) => page.isImportWalletsCheckComplete)

  // computed
  const isImportingFromMetaMaskExtenstion = restoreFrom === 'metamask'
  const isCheckingExtensions = restoreFrom !== 'seed' && !isImportWalletsCheckComplete

  // state
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [metaMaskPassword, setMetaMaskPassword] = React.useState('')
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [phraseInput, setPhraseInput] = React.useState('')
  const [currentStep, setCurrentStep] = React.useState<RestoreFromOtherWalletSteps>(
    isImportingFromMetaMaskExtenstion
      ? RestoreFromOtherWalletSteps.currentPassword
      : RestoreFromOtherWalletSteps.phrase
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
    if (!isPasswordValid) {
      return
    }

    if (isImportingFromMetaMaskExtenstion) {
      dispatch(WalletPageActions.importFromMetaMask({
        password: metaMaskPassword,
        newPassword: password
      }))
      return
    }

    if (invalidMnemonic) {
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
  }, [phraseInput, password, invalidMnemonic, isPasswordValid, isImportingFromMetaMaskExtenstion])

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
    if (currentStep === RestoreFromOtherWalletSteps.currentPassword && metaMaskPassword) {
      // TODO, check import & pw?
      return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
    }
    if (currentStep === RestoreFromOtherWalletSteps.phrase && !invalidMnemonic) {
      return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
    }
    if (currentStep === RestoreFromOtherWalletSteps.newPassword && isPasswordValid) {
      // TODO: check for MM and use alternative restore method?
      return restoreWallet()
    }
  }, [currentStep, invalidMnemonic, isPasswordValid, restoreWallet])

  const onGoBack = React.useCallback(() => {
    if (
      currentStep === RestoreFromOtherWalletSteps.currentPassword ||
      currentStep === RestoreFromOtherWalletSteps.phrase
    ) {
      return history.goBack()
    }
    if (currentStep === RestoreFromOtherWalletSteps.newPassword) {
      return setCurrentStep(RestoreFromOtherWalletSteps.phrase)
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

  const pageText = React.useMemo(() => {
    switch (currentStep) {
      // getLocale
      case RestoreFromOtherWalletSteps.currentPassword: return {
        title: 'We detected the MetaMask extension in your browser', // currently only for metamask import
        description: 'By entering the password you can Import your MetaMask wallet to Brave Wallet easily.'
      }

      // getLocale
      case RestoreFromOtherWalletSteps.newPassword: return {
        title: getLocale('braveWalletCreatePasswordTitle'),
        description: getLocale('braveWalletCreatePasswordDescription')
      }

      // getLocale
      case RestoreFromOtherWalletSteps.phrase: return {
        title: restoreFrom !== 'seed' ? 'Import from MetaMask' : getLocale('braveWalletRecoveryPhraseBackupTitle'),
        description: restoreFrom !== 'seed' ? 'Type your MetaMask 12 words recovery phrase.' : getLocale('braveWalletRecoveryPhraseBackupWarning')
      }

      default: return { title: '', description: '' }
    }
  }, [currentStep, restoreFrom])

  // effects
  React.useEffect(() => {
    if (
      restoreFrom !== 'seed' &&
      !isMetaMaskInitialized &&
      !isImportWalletsCheckComplete
    ) {
      // check if MM is installed
      dispatch(WalletPageActions.checkWalletsToImport())
    }
  }, [restoreFrom, isCheckingExtensions, isMetaMaskInitialized])

  React.useEffect(() => {
    if (
      restoreFrom === 'seed' || // only watching during metamask import
      !isImportWalletsCheckComplete // wait for redux store to be ready
    ) {
      return
    }

    // switch to phrase input if MM was not detected
    if (currentStep === RestoreFromOtherWalletSteps.currentPassword && !isMetaMaskInitialized) {
      return setCurrentStep(RestoreFromOtherWalletSteps.phrase)
    }

    // switch to password input if MM was detected
    if (currentStep === RestoreFromOtherWalletSteps.phrase && isMetaMaskInitialized) {
      return setCurrentStep(RestoreFromOtherWalletSteps.currentPassword)
    }
  }, [isImportWalletsCheckComplete, isImportingFromMetaMaskExtenstion, currentStep])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            steps={restoreFrom === 'metamask' ? RESTORE_FROM_METAMASK_STEPS : RESTORE_FROM_PHRASE_STEPS}
            currentStep={currentStep}
            goBack={onGoBack}
            preventSkipAhead
          />

          <TitleAndDescriptionContainer>
            {
              isCheckingExtensions
              ? <>
                  <Title>{'checking for wallet extensions...'}</Title>
                </>
              : <>
                <Title>{pageText.title}</Title>
                <Description>{pageText.description}</Description>
              </>
            }
          </TitleAndDescriptionContainer>

          {isCheckingExtensions &&
            <>
              <LoadingSkeleton width={375} height={168} />
              <VerticalSpace space={'100px'} />
            </>
          }

          {!isCheckingExtensions && currentStep === RestoreFromOtherWalletSteps.phrase &&
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

          {!isCheckingExtensions && currentStep === RestoreFromOtherWalletSteps.currentPassword &&
            <>
              <PasswordInput
                autoFocus={true}
                onChange={setMetaMaskPassword}
                value={metaMaskPassword}
                error={''}
                hasError={false}
                placeholder={
                  // getLocale
                  'Type MataMask password'
                }
                name='mmPassword'
                label={
                  // getLocale
                  'Password'
                }
              />

              <VerticalSpace space='100px' />
            </>
          }

          {currentStep === RestoreFromOtherWalletSteps.newPassword &&
            <NewPasswordInput
              autoFocus={true}
              onSubmit={restoreWallet}
              onChange={handlePasswordChange}
            />
          }

          {!isCheckingExtensions &&
            <NextButtonRow>
              <NavButton
                buttonType='primary'
                text={getLocale('braveWalletButtonContinue')}
                onSubmit={onContinueClicked}
                disabled={
                  isImportingFromMetaMaskExtenstion && currentStep === RestoreFromOtherWalletSteps.currentPassword && !metaMaskPassword ||
                  restoreFrom === 'seed' && !phraseInput ||
                  currentStep === RestoreFromOtherWalletSteps.phrase && invalidMnemonic ||
                  currentStep === RestoreFromOtherWalletSteps.newPassword && !isPasswordValid
                }
              />
            </NextButtonRow>
          }

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}
