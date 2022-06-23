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
import {
  LoadingIcon,
  ToggleVisibilityButton,
  WalletLink,
  VerticalSpace
} from '../../../../components/shared/style'
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
  PhraseCardTopRow
} from '../onboarding.style'
import { RecoveryTextArea, RecoveryTextInput } from './restore-from-recovery-phrase.style'

// components
import { NewPasswordInput, NewPasswordValues } from '../../../../components/shared/password-input/new-password-input'
import { WalletPageLayout } from '../../../../components/desktop'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'
import { NavButton } from '../../../../components/extension'
import { LoadingSkeleton, PasswordInput } from '../../../../components/shared'
import { ErrorText } from '../../../../components/shared/password-input/style'

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

const RESTORE_FROM_EXTENSION_STEPS: RestoreFromOtherWalletSteps[] = [
  RestoreFromOtherWalletSteps.currentPassword,
  RestoreFromOtherWalletSteps.newPassword,
  RestoreFromOtherWalletSteps.complete
]

interface Props {
  restoreFrom: 'metamask' | 'seed' | 'metamask-seed' | 'legacy'
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
  const isLegacyCryptoWalletsInitialized = useSelector(({ page }: { page: PageState }) => page.isCryptoWalletsInitialized)
  const isImportWalletsCheckComplete = useSelector(({ page }: { page: PageState }) => page.isImportWalletsCheckComplete)
  const importWalletError = useSelector(({ page }: { page: PageState }) => page.importWalletError)
  const importWalletAttempts = useSelector(({ page }: { page: PageState }) => page.importWalletAttempts)

  // computed
  const isImportingFromMetaMaskExtension = restoreFrom === 'metamask'
  const isImportingFromLegacyExtension = restoreFrom === 'legacy'
  const isImportingFromExtension = isImportingFromMetaMaskExtension || isImportingFromLegacyExtension
  const isCheckingExtensions = restoreFrom !== 'seed' && !isImportWalletsCheckComplete

  // state
  const [isCheckingImportPassword, setIsCheckingImportPassword] = React.useState(false)
  const [currentImportAttempt, setCurrentImportAttempt] = React.useState(importWalletAttempts)
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [extensionPassword, setExtensionPassword] = React.useState('')
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [phraseInput, setPhraseInput] = React.useState('')
  const [currentStep, setCurrentStep] = React.useState<RestoreFromOtherWalletSteps>(
    isImportingFromExtension
      ? RestoreFromOtherWalletSteps.currentPassword
      : RestoreFromOtherWalletSteps.phrase
  )

  // methods
  const checkImportPassword = React.useCallback(() => {
    setIsCheckingImportPassword(true)
    setCurrentImportAttempt(importWalletAttempts)

    if (isImportingFromMetaMaskExtension) {
      dispatch(WalletPageActions.importFromMetaMask({
        password: extensionPassword,
        newPassword: '' // required arg, just checking import Password
      }))
      return
    }

    if (isImportingFromLegacyExtension) {
      dispatch(WalletPageActions.importFromCryptoWallets({
        password: extensionPassword,
        newPassword: '' // required arg, just checking import Password
      }))
    }
  }, [isImportingFromMetaMaskExtension, isImportingFromLegacyExtension, extensionPassword, importWalletAttempts])

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

    if (isImportingFromMetaMaskExtension) {
      dispatch(WalletPageActions.importFromMetaMask({
        password: extensionPassword,
        newPassword: password
      }))
      return
    }

    if (isImportingFromLegacyExtension) {
      dispatch(WalletPageActions.importFromCryptoWallets({
        password: extensionPassword,
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
  }, [isPasswordValid, phraseInput, password, invalidMnemonic, isImportingFromMetaMaskExtension, isImportingFromLegacyExtension])

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

  const onExtensionPasswordChange = React.useCallback((value: string): void => {
    if (importWalletError?.hasError) {
      dispatch(WalletPageActions.setImportWalletError({
        hasError: false
      }))
    }
    setExtensionPassword(value)
  }, [importWalletError?.hasError])

  const onContinueClicked = React.useCallback(() => {
    // reset previous errors
    if (importWalletError?.hasError) {
      dispatch(WalletPageActions.setImportWalletError({
        hasError: false
      }))
    }

    if (currentStep === RestoreFromOtherWalletSteps.currentPassword && extensionPassword) {
      return checkImportPassword()
    }
    if (currentStep === RestoreFromOtherWalletSteps.phrase && !invalidMnemonic) {
      return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
    }
    if (currentStep === RestoreFromOtherWalletSteps.newPassword && isPasswordValid) {
      return restoreWallet()
    }
  }, [
    importWalletError?.hasError,
    currentStep, extensionPassword,
    invalidMnemonic,
    isPasswordValid,
    restoreWallet,
    checkImportPassword
  ])

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
  }, [onContinueClicked])

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
      case RestoreFromOtherWalletSteps.currentPassword: return {
        title: restoreFrom === 'metamask'
          ? getLocale('braveWalletMetaMaskExtensionDetected')
          : getLocale('braveWalletCryptoWalletsDetected'),
        description: restoreFrom === 'metamask'
          ? getLocale('braveWalletMetaMaskExtensionImportDescription')
          : getLocale('braveWalletImportBraveLegacyDescription')
      }

      case RestoreFromOtherWalletSteps.newPassword: return {
        title: getLocale('braveWalletCreatePasswordTitle'),
        description: getLocale('braveWalletCreatePasswordDescription')
      }

      case RestoreFromOtherWalletSteps.phrase: return {
        title: restoreFrom !== 'seed'
          ? getLocale('braveWalletImportFromMetaMask')
          : getLocale('braveWalletRestoreMyBraveWallet'),

        description: restoreFrom !== 'seed'
          ? getLocale('braveWalletImportFromMetaMaskSeedInstructions')
          : getLocale('braveWalletRestoreMyBraveWalletInstructions')
      }

      default: return { title: '', description: '' }
    }
  }, [currentStep, restoreFrom])

  // effects
  React.useEffect(() => {
    if (isCheckingExtensions) {
      // check if MM or legacy wallet is installed
      dispatch(WalletPageActions.checkWalletsToImport())
    }
  }, [isCheckingExtensions])

  React.useEffect(() => {
    if (
      restoreFrom === 'seed' || // only watching during metamask or legacy import
      !isImportWalletsCheckComplete // wait for redux store to be ready
    ) {
      return
    }

    // switch to phrase input if MM & legacy wallet was not detected
    if (
      currentStep === RestoreFromOtherWalletSteps.currentPassword &&
      !isMetaMaskInitialized &&
      !isLegacyCryptoWalletsInitialized
    ) {
      return history.push(WalletRoutes.OnboardingRestoreWallet)
    }

    // switch to password input if MM was detected
    if (
      currentStep === RestoreFromOtherWalletSteps.phrase &&
      isMetaMaskInitialized
    ) {
      return setCurrentStep(RestoreFromOtherWalletSteps.currentPassword)
    }
  }, [restoreFrom, isImportWalletsCheckComplete, currentStep, isMetaMaskInitialized, isLegacyCryptoWalletsInitialized])

  React.useEffect(() => {
    // clear other errors on step change
    if (
      currentStep === RestoreFromOtherWalletSteps.newPassword &&
      importWalletError?.hasError &&
      importWalletError.errorMessage
    ) {
      dispatch(WalletPageActions.setImportWalletError({
        hasError: false
      }))
    }
  }, [currentStep, importWalletError])

  React.useEffect(() => {
    // runs after a wallet import password check has completed
    if (
      currentStep === RestoreFromOtherWalletSteps.currentPassword &&
      importWalletAttempts > currentImportAttempt
    ) {
        setCurrentImportAttempt(importWalletAttempts)
        setIsCheckingImportPassword(false)
      // redirect after a successful password check
      if (!importWalletError?.hasError) {
        return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
      }
    }
  }, [currentStep, importWalletAttempts, currentImportAttempt, importWalletError?.hasError])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            steps={isImportingFromExtension ? RESTORE_FROM_EXTENSION_STEPS : RESTORE_FROM_PHRASE_STEPS}
            currentStep={currentStep}
            goBack={onGoBack}
            preventSkipAhead
          />

          <TitleAndDescriptionContainer>
            {
              isCheckingExtensions
              ? <>
                  <Title>
                    {getLocale('braveWalletCheckingInstalledExtensions')}
                  </Title>
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
                onChange={onExtensionPasswordChange}
                value={extensionPassword}
                error={importWalletError?.errorMessage || ''}
                hasError={importWalletError?.hasError}
                onKeyDown={handleKeyDown}
                placeholder={
                  restoreFrom === 'metamask'
                    ? getLocale('braveWalletMetaMaskPasswordInputPlaceholder')
                    : getLocale('braveWalletImportBraveLegacyInput')
                }
                name='extensionPassword'
                label={getLocale('braveWalletInputLabelPassword')}
              />

              <VerticalSpace space='100px' />
            </>
          }

          {currentStep === RestoreFromOtherWalletSteps.newPassword &&
            <>
              <NewPasswordInput
                autoFocus={true}
                onSubmit={restoreWallet}
                onChange={handlePasswordChange}
              />
              {importWalletError?.hasError &&
                <ErrorText>
                  {importWalletError.errorMessage}
                </ErrorText>
              }
            </>
          }

          {!isCheckingExtensions &&
            <NextButtonRow>
              <NavButton
                buttonType='primary'
                text={isCheckingImportPassword
                  ? <LoadingIcon
                    size='24px'
                    opacity={0.8}
                    color='interactive08'
                  /> as unknown as string
                  : getLocale('braveWalletButtonContinue')
                }
                onSubmit={onContinueClicked}
                disabled={
                  isCheckingImportPassword ||
                  isImportingFromExtension && currentStep === RestoreFromOtherWalletSteps.currentPassword && !extensionPassword ||
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
