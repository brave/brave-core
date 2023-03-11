// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'

// actions
import { WalletPageActions } from '../../../actions'

// types
import { PageState, WalletRoutes, WalletState } from '../../../../constants/types'

// styles
import {
  LoadingIcon,
  VerticalSpace,
  CloseIcon,
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
import LoadingSkeleton from '../../../../components/shared/loading-skeleton/index'
import { PasswordInput } from '../../../../components/shared/password-input/index'
import {
  NewPasswordInput,
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import { NavButton } from '../../../../components/extension'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'
import { RecoveryInput } from './recovery-input'
import { Checkbox } from '../../../../components/shared/checkbox/checkbox'

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
  restoreFrom: 'metamask'
  | 'metamask-seed'
  | 'legacy'
  | 'legacy-seed'
  | 'seed'
}

export const OnboardingRestoreFromRecoveryPhrase = ({
  restoreFrom = 'seed'
}: Props) => {
  // routing
  let history = useHistory()

  // redux
  const dispatch = useDispatch()
  const invalidMnemonic = useSelector(({ page }: { page: PageState }) => page.invalidMnemonic)
  const isImportWalletsCheckComplete = useSelector(({ page }: { page: PageState }) => page.isImportWalletsCheckComplete)
  const importWalletError = useSelector(({ page }: { page: PageState }) => page.importWalletError)
  const importWalletAttempts = useSelector(({ page }: { page: PageState }) => page.importWalletAttempts)
  const isWalletCreated = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletCreated)

  // computed
  const isImportingFromMetaMaskExtension = restoreFrom === 'metamask'
  const isImportingFromLegacyExtension = restoreFrom === 'legacy'
  const isImportingFromExtension = isImportingFromMetaMaskExtension || isImportingFromLegacyExtension
  const isCheckingExtensions = !restoreFrom.includes('seed') && !isImportWalletsCheckComplete

  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [isCheckingImportPassword, setIsCheckingImportPassword] = React.useState(false)
  const [currentImportAttempt, setCurrentImportAttempt] = React.useState(importWalletAttempts)
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [extensionPassword, setExtensionPassword] = React.useState('')
  const [phraseInput, setPhraseInput] = React.useState('')
  const [phraseWordsLength, setPhraseWordsLength] = React.useState(0)
  const [isImportingFromLegacySeed, setIsImportingFromLegacySeed] = React.useState(false)
  const [currentStep, setCurrentStep] = React.useState<RestoreFromOtherWalletSteps>(
    isImportingFromExtension
      ? RestoreFromOtherWalletSteps.currentPassword
      : RestoreFromOtherWalletSteps.phrase
  )

  // 12, 15, 18, 21, or 24
  const isCorrectPhraseLength =
    phraseWordsLength === 12 ||
    phraseWordsLength === 15 ||
    phraseWordsLength === 18 ||
    phraseWordsLength === 21 ||
    phraseWordsLength === 24

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
  }, [
    isImportingFromMetaMaskExtension,
    isImportingFromLegacyExtension,
    extensionPassword,
    importWalletAttempts
  ])

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
      isLegacy: isImportingFromLegacySeed,
      completeWalletSetup: false // postpone until wallet onboarding success screen
    }))
  }, [
    isPasswordValid,
    isImportingFromMetaMaskExtension,
    extensionPassword,
    password,
    isImportingFromLegacyExtension,
    phraseInput,
    invalidMnemonic,
    isImportingFromLegacySeed
  ])

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

    if (currentStep === RestoreFromOtherWalletSteps.phrase) {
      return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
    }

    if (currentStep === RestoreFromOtherWalletSteps.newPassword && isPasswordValid) {
      return restoreWallet()
    }
  }, [
    importWalletError?.hasError,
    currentStep,
    extensionPassword,
    invalidMnemonic,
    isPasswordValid,
    restoreWallet,
    checkImportPassword
  ])

  const onGoBack = React.useCallback(() => {
    // clear errors
    if (invalidMnemonic) {
      dispatch(WalletPageActions.hasMnemonicError(false))
    }

    if (
      currentStep === RestoreFromOtherWalletSteps.currentPassword ||
      currentStep === RestoreFromOtherWalletSteps.phrase
    ) {
      return history.goBack()
    }

    if (currentStep === RestoreFromOtherWalletSteps.newPassword) {
      setPhraseInput('') // reset input state
      return setCurrentStep(RestoreFromOtherWalletSteps.phrase)
    }
  }, [invalidMnemonic, currentStep])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLElement>) => {
    if (event.key === 'Enter') {
      onContinueClicked()
    }
  }, [onContinueClicked])

  const onRecoveryInputChanged = React.useCallback(({ value, phraseLength }: { value: string, isValid: boolean, phraseLength: number }): void => {
    setPhraseInput(value)
    setPhraseWordsLength(phraseLength)
  }, [])

  // memos
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
        title: restoreFrom.includes('metamask')
          ? getLocale('braveWalletImportFromMetaMask')
          : getLocale('braveWalletRestoreMyBraveWallet'),

        description: restoreFrom.includes('metamask')
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
    // go to onboarding success screen when wallet restoration completes
    if (isWalletCreated) {
      history.push(WalletRoutes.OnboardingComplete)
    }
  }, [isWalletCreated])

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
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <StepsNavigation
            steps={isImportingFromExtension ? RESTORE_FROM_EXTENSION_STEPS : RESTORE_FROM_PHRASE_STEPS}
            currentStep={currentStep}
            goBack={onGoBack}
            preventSkipAhead
            onSkip={!restoreFrom.includes('seed') ? () => {
              return restoreFrom.includes('metamask')
                ? history.push(WalletRoutes.OnboardingImportMetaMaskSeed)
                : history.push(WalletRoutes.OnboardingImportCryptoWalletsSeed)
            } : undefined}
            skipButtonText={<CloseIcon />}
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
                <RecoveryInput
                  onChange={onRecoveryInputChanged}
                  onKeyDown={handleKeyDown}
                  onToggleShowPhrase={setIsPhraseShown}
                />
              </PhraseCard>

              <VerticalSpace space={isPhraseShown ? '20px' : '130px'} />

              <Row alignItems='center' style={{ minHeight: 53 }}>
                {phraseWordsLength === 24 &&
                  <Checkbox isChecked={isImportingFromLegacySeed} onChange={setIsImportingFromLegacySeed} disabled={false}>
                    <Description>
                      {getLocale('braveWalletRestoreLegacyCheckBox')}
                    </Description>
                  </Checkbox>
                }

                {phraseInput && phraseWordsLength > 12 && !isCorrectPhraseLength &&
                  <ErrorText>
                    {getLocale('braveWalletRecoveryPhraseLengthError')}
                  </ErrorText>
                }
              </Row>
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
              {!!invalidMnemonic &&
                <ErrorText>
                  {getLocale('braveWalletInvalidMnemonicError')}
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
                  currentStep === RestoreFromOtherWalletSteps.phrase && (!phraseInput || phraseWordsLength < 12 || (phraseWordsLength > 12 && !isCorrectPhraseLength)) ||
                  currentStep === RestoreFromOtherWalletSteps.newPassword && (!isPasswordValid || invalidMnemonic)
                }
              />
            </NextButtonRow>
          }

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
