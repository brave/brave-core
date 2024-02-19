// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'
import Input from '@brave/leo/react/input'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'

// utils
import { getLocale } from '../../../../../common/locale'
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
import {
  useReportOnboardingActionMutation,
  useRestoreWalletMutation,
  useSetAutoLockMinutesMutation
} from '../../../../common/slices/api.slice'

// components
import NewPasswordInput, {
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'
import { AutoLockSettings } from '../components/auto-lock-settings/auto-lock-settings'
import { CreatingWallet } from '../creating_wallet/creating_wallet'

// options
import { autoLockOptions } from '../../../../options/auto-lock-options'

// styles
import { Column, VerticalSpace } from '../../../../components/shared/style'
import {
  ErrorAlert,
  RecoveryPhraseContainer
} from './restore-from-recovery-phrase-v3.style'
import { ContinueButton } from '../onboarding.style'

type RestoreWalletSteps = 'phrase' | 'password'
const VALID_PHRASE_LENGTHS = [12, 15, 18, 21, 24]

export const OnboardingRestoreFromRecoveryPhrase = () => {
  // mutations
  const [restoreWalletFromSeed, { isLoading: isCreatingWallet }] =
    useRestoreWalletMutation()
  const [report] = useReportOnboardingActionMutation()
  const [setAutoLockMinutes] = useSetAutoLockMinutesMutation()

  // routing
  const history = useHistory()

  // state
  const [recoveryPhraseLength, setRecoveryPhraseLength] = React.useState(12)
  const [revealPhrase, setRevealPhrase] = React.useState(false)
  const [phraseWords, setPhraseWords] = React.useState<string[]>([])
  const [currentStep, setCurrentStep] =
    React.useState<RestoreWalletSteps>('phrase')
  const [isPasswordValid, setIsPasswordValid] = React.useState(false)
  const [hasInvalidSeedError, setHasInvalidSeedError] = React.useState(false)
  const [password, setPassword] = React.useState('')
  const [autoLockDuration, setAutoLockDuration] = React.useState(
    autoLockOptions[0].value
  )

  const isCorrectPhraseLength = VALID_PHRASE_LENGTHS.includes(
    phraseWords.length
  )

  // memos
  const alternateRecoveryPhraseLength = React.useMemo(
    () => (recoveryPhraseLength === 12 ? 24 : 12),
    [recoveryPhraseLength]
  )

  const recoveryPhrase = React.useMemo(
    () => phraseWords.join(' '),
    [phraseWords]
  )

  // methods
  const onPhraseWordChange = React.useCallback(
    (index: number, value: string) => {
      if (value.includes(' ')) {
        const words = value.split(' ')
        setPhraseWords((prev) => {
          const newValues = [...prev]
          words.forEach((word, i) => {
            if (i < recoveryPhraseLength) {
              newValues[i] = word
            }
          })
          return newValues
        })
      } else {
        setPhraseWords((prev) => {
          const newValues = [...prev]
          newValues[index] = value
          return newValues
        })
      }
    },
    [recoveryPhraseLength]
  )

  const onRecoveryPhraseLengthChange = React.useCallback(() => {
    setRecoveryPhraseLength(alternateRecoveryPhraseLength)
    setPhraseWords([])
  }, [alternateRecoveryPhraseLength])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsPasswordValid(isValid)
    },
    []
  )

  const restoreWallet = React.useCallback(async () => {
    if (!isPasswordValid) {
      return
    }

    const { invalidMnemonic } = await restoreWalletFromSeed({
      // added an additional trim here in case the phrase length is
      // 12, 15, 18 or 21 long and has a space at the end.
      mnemonic: recoveryPhrase.trimEnd(),
      password,
      isLegacy: recoveryPhraseLength === 24,
      // postpone until wallet onboarding success screen
      completeWalletSetup: false
    }).unwrap()

    setHasInvalidSeedError(invalidMnemonic)

    if (!invalidMnemonic) {
      await setAutoLockMinutes(autoLockDuration)
      history.push(WalletRoutes.OnboardingComplete)
    }
  }, [isPasswordValid, recoveryPhrase, password, restoreWalletFromSeed])

  const onContinue = React.useCallback(async () => {
    if (currentStep === 'phrase') {
      return setCurrentStep('password')
    }

    if (currentStep === 'password' && isPasswordValid) {
      return await restoreWallet()
    }
  }, [currentStep, isPasswordValid])

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
    <OnboardingContentLayout
      title={getLocale(
        currentStep === 'phrase'
          ? 'braveWalletRestoreMyBraveWallet'
          : 'braveWalletCreatePasswordTitle'
      )}
      subTitle={getLocale(
        currentStep === 'phrase'
          ? 'braveWalletRestoreMyBraveWalletInstructions'
          : 'braveWalletCreatePasswordDescription'
      )}
    >
      {currentStep === 'phrase' && (
        <>
          <VerticalSpace space='52px' />
          <RecoveryPhraseContainer phraseLength={recoveryPhraseLength}>
            {Array.from({ length: recoveryPhraseLength }, (_, index) => (
              <Input
                key={index}
                placeholder={`Word #${index + 1}`}
                value={phraseWords[index] || ''}
                size='small'
                onChange={(e) => onPhraseWordChange(index, e.detail.value)}
                onInput={(e) => onPhraseWordChange(index, e.detail.value)}
                type={revealPhrase ? 'text' : 'password'}
              />
            ))}
          </RecoveryPhraseContainer>
          <Column alignItems='flex-end'>
            <Button
              kind='plain'
              onClick={() => setRevealPhrase((reveal) => !reveal)}
            >
              <Icon name={revealPhrase ? 'eye-off' : 'eye-on'} />
            </Button>
          </Column>
          <VerticalSpace space='28px' />
          <Button
            kind='plain'
            onClick={onRecoveryPhraseLengthChange}
            color={leo.color.container.interactive}
          >
            {getLocale('braveWalletRestoreAlternateLength').replace(
              '$1',
              alternateRecoveryPhraseLength.toString()
            )}
          </Button>
          <VerticalSpace space='12px' />
          {(phraseWords.length > 0 && !isCorrectPhraseLength) ||
          hasInvalidSeedError ? (
            <ErrorAlert>
              {getLocale('braveWalletRestoreWalletError')}
            </ErrorAlert>
          ) : (
            <VerticalSpace space='54px' />
          )}
          <VerticalSpace space='24px' />
        </>
      )}

      {/* Create Password */}
      {currentStep === 'password' && (
        <>
          <VerticalSpace space='68px' />

          <NewPasswordInput
            autoFocus={true}
            onSubmit={onContinue}
            onChange={handlePasswordChange}
          />

          <VerticalSpace space='68px' />

          <AutoLockSettings
            options={autoLockOptions}
            value={autoLockDuration}
            onChange={setAutoLockDuration}
          />
          <VerticalSpace space='24px' />
        </>
      )}

      <Column>
        <ContinueButton
          isDisabled={
            !recoveryPhrase ||
            (currentStep === 'phrase' &&
              (!recoveryPhrase ||
                recoveryPhraseLength < 12 ||
                (recoveryPhraseLength > 12 && !isCorrectPhraseLength))) ||
            (currentStep === 'password' &&
              (!isPasswordValid || hasInvalidSeedError))
          }
          onClick={onContinue}
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </Column>
    </OnboardingContentLayout>
  )
}
