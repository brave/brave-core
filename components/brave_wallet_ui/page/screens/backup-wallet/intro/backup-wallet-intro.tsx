// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// components
import { Checkbox } from 'brave-ui'
import NavButton from '../../../../components/extension/buttons/nav-button/index'
import PasswordInput from '../../../../components/shared/password-input/index'

// hooks
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'

// style
import {
  StyledWrapper,
  Title,
  Description,
  TermsRow,
  SkipButton,
  PageIcon,
  PasswordColumn
} from './backup-wallet-intro.style'
import { InputLabelText } from '../../../../components/desktop/popup-modals/account-settings-modal/account-settings-modal.style'
import { useDispatch } from 'react-redux'
import { WalletPageActions } from '../../../actions'

export interface Props {
  onSubmit: () => void
  isBackupTermsAccepted: boolean
  isOnboarding: boolean
  onSubmitTerms: (key: string, selected: boolean) => void
  onCancel: () => void
  recoveryPhraseLength: number
}

export const BackupWalletIntroStep = ({
  onSubmit,
  isBackupTermsAccepted,
  isOnboarding,
  onSubmitTerms,
  onCancel,
  recoveryPhraseLength
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // state
  const [password, setPassword] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] = React.useState<boolean>(true)

  const hasPasswordError = isOnboarding
    ? false // password not needed during onboarding
    : !password || !isCorrectPassword // needs correct value

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // methods
  const onPasswordChange = (value: string): void => {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }

  const onContinue = async () => {
    // not password-protected during onboarding
    if (isOnboarding) {
      return onSubmit()
    }

    if (!password) { // require password to view key
      return
    }

    // entered password must be correct
    const isPasswordValid = await attemptPasswordEntry(password)

    if (!isPasswordValid) {
      setIsCorrectPassword(isPasswordValid) // set or clear error
      return // need valid password to continue
    }

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    // reveal recovery phrase
    dispatch(WalletPageActions.showRecoveryPhrase({ show: true, password }))

    // continue on
    onSubmit()
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onContinue()
    }
  }

  // render
  return (
    <StyledWrapper>
      <PageIcon />

      <Title>{getLocale('braveWalletBackupIntroTitle')}</Title>
      <Description>{getLocale('braveWalletBackupIntroDescription').replace('$1', recoveryPhraseLength.toString())}</Description>

      <TermsRow>
        <Checkbox
          value={{ backupTerms: isBackupTermsAccepted }}
          onChange={onSubmitTerms}
        >
          <div onKeyDown={handleKeyDown} data-key='backupTerms'>
            {getLocale('braveWalletBackupIntroTerms')}
          </div>
        </Checkbox>
      </TermsRow>

      {!isOnboarding &&
        <PasswordColumn>
          <InputLabelText>{getLocale('braveWalletEnterYourPassword')}</InputLabelText>
          <PasswordInput
            placeholder={getLocale('braveWalletCreatePasswordInput')}
            onChange={onPasswordChange}
            error={getLocale('braveWalletLockScreenError')}
            hasError={!!password && !isCorrectPassword}
            onKeyDown={handleKeyDown}
            value={password}
            autoFocus={false}
          />
        </PasswordColumn>
      }
      <NavButton
        disabled={!isBackupTermsAccepted || hasPasswordError}
        buttonType='primary'
        text={getLocale('braveWalletButtonContinue')}
        onSubmit={onContinue}
      />

      <SkipButton onClick={onCancel}>
        {getLocale(isOnboarding ? 'braveWalletButtonSkip' : 'braveWalletButtonCancel')}
      </SkipButton>
    </StyledWrapper>
  )
}

export default BackupWalletIntroStep
