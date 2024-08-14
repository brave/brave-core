// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'
import Input from '@brave/leo/react/input'
import Icon from '@brave/leo/react/icon'
import Checkbox from '@brave/leo/react/checkbox'
import * as leo from '@brave/leo/tokens/css/variables'

// utils
import { getLocale } from '../../../../../common/locale'
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
import {
  useGetWalletsToImportQuery,
  useReportOnboardingActionMutation,
  useRestoreWalletMutation,
  useSetAutoLockMinutesMutation
} from '../../../../common/slices/api.slice'
import { clearClipboard } from '../../../../utils/copy-to-clipboard'
import {
  normalizeRecoveryPhraseInput,
  WORD_SEPARATOR
} from '../../../../utils/recovery-phrase-utils'

// components
import {
  NewPasswordValues //
} from '../../../../components/shared/password-input/new-password-input'
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'
import {
  OnboardingCreatingWallet //
} from '../creating_wallet/onboarding_creating_wallet'
import { CreatePassword } from '../create_password/components/create_password'
import { AutoLockSettings } from '../components/auto_lock_settings/auto_lock_settings'

// options
import { autoLockOptions } from '../../../../options/auto_lock_options'

// styles
import { Column, Row } from '../../../../components/shared/style'
import {
  CheckboxText,
  InfoAlert,
  RecoveryPhraseContainer
} from './restore-from-recovery-phrase.style'
import { AlertWrapper, ContinueButton } from '../onboarding.style'

type RestoreWalletSteps = 'phrase' | 'password'
const VALID_PHRASE_LENGTHS = [12, 15, 18, 21, 24]

export const OnboardingRestoreFromRecoveryPhrase = () => {
  // queries
  const { data: importableWallets } = useGetWalletsToImportQuery()

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
    autoLockOptions[0].minutes
  )
  const [isImportingFromLegacySeed, setIsImportingFromLegacySeed] =
    React.useState(false)

  const isCorrectPhraseLength = VALID_PHRASE_LENGTHS.includes(
    phraseWords.length
  )

  const alternateRecoveryPhraseLength = recoveryPhraseLength === 12 ? 24 : 12
  const recoveryPhrase = phraseWords.join(' ')

  let title = ''
  let subTitle = ''

  switch (currentStep) {
    case 'phrase':
      title = getLocale('braveWalletRestoreMyBraveWallet')
      subTitle = getLocale('braveWalletRestoreMyBraveWalletInstructions')
      break
    case 'password':
      title = getLocale('braveWalletCreatePasswordTitle')
      subTitle = getLocale('braveWalletCreatePasswordDescription')
      break
    default:
      title = ''
      subTitle = ''
      break
  }

  // methods
  const onPhraseWordChange = React.useCallback(
    async (index: number, value: string) => {
      const sanitizedValue = normalizeRecoveryPhraseInput(value)
      // when the a recovery phrase is pasted,
      // split the words and fill the input fields
      if (sanitizedValue.includes(WORD_SEPARATOR)) {
        const words = sanitizedValue.split(WORD_SEPARATOR)
        let newPhraseLength = words.length > 12 ? 24 : 12
        if (newPhraseLength !== recoveryPhraseLength) {
          setRecoveryPhraseLength(newPhraseLength)
        }

        setPhraseWords((prev) => {
          const newValues = [...prev]
          words.forEach((word, i) => {
            if (i < newPhraseLength && !newValues[i]) {
              newValues[i] = word
            }
          })
          return newValues
        })

        // clear clipboard
        await clearClipboard()
      } else {
        // update the word at the index with the new value entered
        setPhraseWords((prev) => {
          const newValues = [...prev]
          newValues[index] = value
          return newValues
        })
      }

      // clear any previous errors
      setHasInvalidSeedError(false)
    },
    [recoveryPhraseLength]
  )

  const onRecoveryPhraseLengthChange = React.useCallback(() => {
    setRecoveryPhraseLength((prev) => (prev === 12 ? 24 : 12))
    setPhraseWords([])
  }, [])

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
      isLegacy: isImportingFromLegacySeed,
      // postpone until wallet onboarding success screen
      completeWalletSetup: false
    }).unwrap()

    setHasInvalidSeedError(invalidMnemonic)

    if (!invalidMnemonic) {
      await setAutoLockMinutes(autoLockDuration)
      history.push(WalletRoutes.OnboardingComplete)
    }
  }, [
    isPasswordValid,
    password,
    isImportingFromLegacySeed,
    restoreWalletFromSeed,
    recoveryPhrase,
    autoLockDuration,
    history,
    setAutoLockMinutes
  ])

  const onContinue = React.useCallback(async () => {
    if (currentStep === 'phrase') {
      return setCurrentStep('password')
    }

    if (currentStep === 'password' && isPasswordValid) {
      return await restoreWallet()
    }
  }, [currentStep, isPasswordValid, restoreWallet])

  // effects
  React.useEffect(() => {
    if (currentStep === 'password') {
      report(BraveWallet.OnboardingAction.LegalAndPassword)
    }
  }, [report, currentStep])

  // render
  if (isCreatingWallet) {
    return <OnboardingCreatingWallet />
  }

  return (
    <OnboardingContentLayout
      title={title}
      subTitle={subTitle}
    >
      {currentStep === 'phrase' && (
        <>
          <Column
            alignItems='flex-end'
            justifyContent='center'
            margin='35px 0 24px'
          >
            <RecoveryPhraseContainer phraseLength={recoveryPhraseLength}>
              {Array.from({ length: recoveryPhraseLength }, (_, index) => (
                <Input
                  key={index}
                  mode='filled'
                  placeholder={getLocale(
                    'braveWalletRecoveryPhraseWord'
                  ).replace('$1', `#${index + 1}`)}
                  value={phraseWords[index] || ''}
                  size='small'
                  onChange={(e) => onPhraseWordChange(index, e.value)}
                  onInput={(e) => onPhraseWordChange(index, e.value)}
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
          </Column>
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
          <Row
            alignItems='center'
            justifyContent='center'
            margin='12px 0 12px'
          >
            {recoveryPhraseLength === 24 ? (
              <Column
                justifyContent='center'
                alignItems='center'
              >
                <Checkbox
                  checked={isImportingFromLegacySeed}
                  onChange={() =>
                    setIsImportingFromLegacySeed(
                      (isImportingFromLegacySeed) => !isImportingFromLegacySeed
                    )
                  }
                >
                  <CheckboxText>
                    {getLocale('braveWalletRestoreLegacyCheckBox')}
                  </CheckboxText>
                </Checkbox>
              </Column>
            ) : null}
          </Row>

          {importableWallets?.isMetaMaskInitialized && (
            <AlertWrapper>
              <InfoAlert padding='0 0 16px 0'>
                <div slot='icon'>
                  <Icon name='metamask-color' />
                </div>
                {getLocale('braveWalletMetamaskDetected')}
                <div slot='content-after'>
                  <Button
                    kind='plain'
                    size='small'
                    onClick={() =>
                      history.push(WalletRoutes.OnboardingImportMetaMask)
                    }
                  >
                    {getLocale('braveWalletUsePassword')}
                  </Button>
                </div>
              </InfoAlert>
            </AlertWrapper>
          )}

          {importableWallets?.isLegacyCryptoWalletsInitialized && (
            <AlertWrapper>
              <InfoAlert>
                <div slot='icon'>
                  <Icon name='crypto-wallets' />
                </div>
                {getLocale('braveWalletLegacyWalletDetected')}
                <div slot='content-after'>
                  <Button
                    kind='plain'
                    size='small'
                    onClick={() =>
                      history.push(WalletRoutes.OnboardingImportLegacy)
                    }
                  >
                    {getLocale('braveWalletUsePassword')}
                  </Button>
                </div>
              </InfoAlert>
            </AlertWrapper>
          )}
        </>
      )}

      {/* Create Password */}
      {currentStep === 'password' && (
        <Column
          alignItems='center'
          justifyContent='center'
          margin='36px 0 24px'
        >
          <Column gap='68px'>
            <CreatePassword
              onPasswordChange={handlePasswordChange}
              onSubmit={onContinue}
              initialPassword={password} // Use the preserved password
            />

            <AutoLockSettings
              options={autoLockOptions}
              value={autoLockDuration}
              onChange={setAutoLockDuration}
            />
          </Column>

          {hasInvalidSeedError && (
            <InfoAlert
              padding='16px 0 0'
              type='error'
            >
              {getLocale('braveWalletRestoreWalletError')}
            </InfoAlert>
          )}
        </Column>
      )}

      <Column padding='24px 0 0 0'>
        <ContinueButton
          isDisabled={
            !recoveryPhrase ||
            (currentStep === 'phrase' &&
              (!recoveryPhrase ||
                !isCorrectPhraseLength ||
                phraseWords.length !== recoveryPhraseLength)) ||
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
