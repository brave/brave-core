// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useCheckExternalWalletPasswordMutation,
  useGetWalletsToImportQuery,
  useImportFromCryptoWalletsMutation,
  useImportFromMetaMaskMutation,
  useRestoreWalletMutation
} from '../../../../common/slices/api.slice'

// types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

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
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'
import { RecoveryInput } from './recovery-input'
import { Checkbox } from '../../../../components/shared/checkbox/checkbox'
import { CreatingWallet } from '../creating_wallet/creating_wallet'

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

const VALID_PHRASE_LENGTHS = [12, 15, 18, 21, 24]

interface Props {
  restoreFrom: 'metamask' | 'metamask-seed' | 'legacy' | 'legacy-seed' | 'seed'
}

export const OnboardingRestoreFromRecoveryPhrase = ({
  restoreFrom = 'seed'
}: Props) => {
  // computed from props
  const isImportingFromMetaMaskExtension = restoreFrom === 'metamask'
  const isImportingFromLegacyExtension = restoreFrom === 'legacy'
  const isImportingFromExtension =
    isImportingFromMetaMaskExtension || isImportingFromLegacyExtension

  // routing
  let history = useHistory()

  // queries
  const { isFetching: isCheckingExtensions } = useGetWalletsToImportQuery(
    isImportingFromExtension ? undefined : skipToken
  )

  // mutations
  const [
    importFromCryptoWallets,
    {
      isLoading: isCreatingWalletFromCryptoWallets,
      data: importFromLegacyWalletResult
    }
  ] = useImportFromCryptoWalletsMutation()

  const [
    importFromMetaMask,
    {
      isLoading: isCreatingWalletFromMetaMaskExtension,
      data: importFromMetaMaskResult
    }
  ] = useImportFromMetaMaskMutation()

  const [restoreWalletFromSeed, { isLoading: isCreatingWalletFromSeed }] =
    useRestoreWalletMutation()

  const [checkExtensionPassword, { isLoading: isCheckingImportPassword }] =
    useCheckExternalWalletPasswordMutation()

  // computed from mutations
  const isCreatingWallet =
    isCreatingWalletFromCryptoWallets ||
    isCreatingWalletFromMetaMaskExtension ||
    isCreatingWalletFromSeed

  // state
  const [hasInvalidSeedError, setHasInvalidSeedError] = React.useState(false)
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [extensionPassword, setExtensionPassword] = React.useState('')
  const [phraseInput, setPhraseInput] = React.useState('')
  const [phraseWordsLength, setPhraseWordsLength] = React.useState(0)
  const [isImportingFromLegacySeed, setIsImportingFromLegacySeed] =
    React.useState(false)
  const [currentStep, setCurrentStep] =
    React.useState<RestoreFromOtherWalletSteps>(
      isImportingFromExtension
        ? RestoreFromOtherWalletSteps.currentPassword
        : RestoreFromOtherWalletSteps.phrase
    )

  const importWalletError = isImportingFromExtension
    ? importFromMetaMaskResult?.errorMessage
    : isImportingFromLegacyExtension
    ? importFromLegacyWalletResult?.errorMessage
    : hasInvalidSeedError
    ? getLocale('braveWalletInvalidMnemonicError')
    : undefined

  const isCorrectPhraseLength = VALID_PHRASE_LENGTHS.includes(phraseWordsLength)

  // methods
  const checkImportPassword = React.useCallback(async () => {
    if (isImportingFromMetaMaskExtension || isImportingFromLegacyExtension) {
      const results = await checkExtensionPassword({
        walletType: isImportingFromMetaMaskExtension
          ? BraveWallet.ExternalWalletType.MetaMask
          : BraveWallet.ExternalWalletType.CryptoWallets,
        password: extensionPassword
      }).unwrap()
      if (results.success) {
        // TODO: clear wrong password error
        return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
      }
      // TODO: show error if wrong password entered
    }
  }, [
    isImportingFromMetaMaskExtension,
    isImportingFromLegacyExtension,
    extensionPassword
  ])

  const restoreWallet = React.useCallback(async () => {
    if (!isPasswordValid) {
      return
    }

    if (isImportingFromMetaMaskExtension) {
      await importFromMetaMask({
        password: extensionPassword,
        newPassword: password
      })
      return
    }

    if (isImportingFromLegacyExtension) {
      await importFromCryptoWallets({
        password: extensionPassword,
        newPassword: password
      })
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
  }, [
    isPasswordValid,
    isImportingFromMetaMaskExtension,
    extensionPassword,
    password,
    isImportingFromLegacyExtension,
    phraseInput,
    isImportingFromLegacySeed
  ])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsPasswordValid(isValid)
    },
    []
  )

  const onContinueClicked = React.useCallback(async () => {
    if (
      currentStep === RestoreFromOtherWalletSteps.currentPassword &&
      extensionPassword
    ) {
      return await checkImportPassword()
    }

    if (currentStep === RestoreFromOtherWalletSteps.phrase) {
      return setCurrentStep(RestoreFromOtherWalletSteps.newPassword)
    }

    if (
      currentStep === RestoreFromOtherWalletSteps.newPassword &&
      isPasswordValid
    ) {
      return await restoreWallet()
    }
  }, [
    currentStep,
    extensionPassword,
    isPasswordValid,
    restoreWallet,
    checkImportPassword
  ])

  // TODO: test all paths
  const onGoBack = React.useCallback(() => {
    setHasInvalidSeedError(false)

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
  }, [currentStep])

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

  // memos
  const pageText = React.useMemo(() => {
    switch (currentStep) {
      case RestoreFromOtherWalletSteps.currentPassword:
        return {
          title:
            restoreFrom === 'metamask'
              ? getLocale('braveWalletMetaMaskExtensionDetected')
              : getLocale('braveWalletCryptoWalletsDetected'),
          description:
            restoreFrom === 'metamask'
              ? getLocale('braveWalletMetaMaskExtensionImportDescription')
              : getLocale('braveWalletImportBraveLegacyDescription')
        }

      case RestoreFromOtherWalletSteps.newPassword:
        return {
          title: getLocale('braveWalletCreatePasswordTitle'),
          description: getLocale('braveWalletCreatePasswordDescription')
        }

      case RestoreFromOtherWalletSteps.phrase:
        return {
          title: restoreFrom.includes('metamask')
            ? getLocale('braveWalletImportFromMetaMask')
            : getLocale('braveWalletRestoreMyBraveWallet'),

          description: restoreFrom.includes('metamask')
            ? getLocale('braveWalletImportFromMetaMaskSeedInstructions')
            : getLocale('braveWalletRestoreMyBraveWalletInstructions')
        }

      default:
        return { title: '', description: '' }
    }
  }, [currentStep, restoreFrom])

  if (isCreatingWallet) {
    return <CreatingWallet />
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <StepsNavigation
            steps={
              isImportingFromExtension
                ? RESTORE_FROM_EXTENSION_STEPS
                : RESTORE_FROM_PHRASE_STEPS
            }
            currentStep={currentStep}
            goBack={onGoBack}
            preventSkipAhead
            onSkip={
              !restoreFrom.includes('seed')
                ? () => {
                    // TODO: why was this needed?
                    // Can we use regular "restore" page
                    return restoreFrom.includes('metamask')
                      ? history.push(WalletRoutes.OnboardingImportMetaMaskSeed)
                      : history.push(
                          WalletRoutes.OnboardingImportCryptoWalletsSeed
                        )
                  }
                : undefined
            }
            skipButtonText={<CloseIcon />}
          />

          <TitleAndDescriptionContainer>
            {isCheckingExtensions ? (
              <>
                <Title>
                  {getLocale('braveWalletCheckingInstalledExtensions')}
                </Title>
              </>
            ) : (
              <>
                <Title>{pageText.title}</Title>
                <Description>{pageText.description}</Description>
              </>
            )}
          </TitleAndDescriptionContainer>

          {isCheckingExtensions && (
            <>
              <LoadingSkeleton
                width={375}
                height={168}
              />
              <VerticalSpace space={'100px'} />
            </>
          )}

          {!isCheckingExtensions &&
            currentStep === RestoreFromOtherWalletSteps.phrase && (
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

          {!isCheckingExtensions &&
            currentStep === RestoreFromOtherWalletSteps.currentPassword && (
              <>
                <PasswordInput
                  autoFocus={true}
                  onChange={setExtensionPassword}
                  value={extensionPassword}
                  error={importWalletError || ''}
                  hasError={!!importWalletError}
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
            )}

          {currentStep === RestoreFromOtherWalletSteps.newPassword && (
            <>
              <NewPasswordInput
                autoFocus={true}
                onSubmit={restoreWallet}
                onChange={handlePasswordChange}
              />
              {importWalletError && <ErrorText>{importWalletError}</ErrorText>}
            </>
          )}

          {!isCheckingExtensions && (
            <NextButtonRow>
              <NavButton
                buttonType='primary'
                text={
                  isCheckingImportPassword
                    ? ((
                        <LoadingIcon
                          size='24px'
                          opacity={0.8}
                          color='interactive08'
                        />
                      ) as unknown as string)
                    : getLocale('braveWalletButtonContinue')
                }
                onSubmit={onContinueClicked}
                disabled={
                  isCheckingImportPassword ||
                  (isImportingFromExtension &&
                    currentStep ===
                      RestoreFromOtherWalletSteps.currentPassword &&
                    !extensionPassword) ||
                  (restoreFrom === 'seed' && !phraseInput) ||
                  (currentStep === RestoreFromOtherWalletSteps.phrase &&
                    (!phraseInput ||
                      phraseWordsLength < 12 ||
                      (phraseWordsLength > 12 && !isCorrectPhraseLength))) ||
                  (currentStep === RestoreFromOtherWalletSteps.newPassword &&
                    (!isPasswordValid || hasInvalidSeedError))
                }
              />
            </NextButtonRow>
          )}
        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
