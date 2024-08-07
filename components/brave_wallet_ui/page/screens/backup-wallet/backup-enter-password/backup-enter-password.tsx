// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
import getAPIProxy from '../../../../common/async/bridge'
import { getLocale } from '../../../../../common/locale'
import { WalletPageActions } from '../../../actions'

// routes
import { WalletRoutes } from '../../../../constants/types'

// hooks
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'

// components
import {
  PasswordInput //
} from '../../../../components/shared/password-input/password-input-v2'

// style
import { Column, Row } from '../../../../components/shared/style'
import {
  NextButtonRow,
  ContinueButton
} from '../../onboarding/onboarding.style'
import { OnboardingContentLayout } from '../../onboarding/components/onboarding_content_layout/content_layout'
import {
  InputLabel //
} from '../../../../components/shared/password-input/password-input-v2.style'

export const BackupEnterPassword: React.FC = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // state
  const [password, setPassword] = React.useState('')
  const [isCorrectPassword, setIsCorrectPassword] =
    React.useState<boolean>(true)

  // memos

  // methods
  const onSubmit = async () => {
    if (!password) {
      // require password to continue
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

    const { mnemonic } = await getAPIProxy().keyringService.getWalletMnemonic(
      password
    )
    if (mnemonic) {
      dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic }))
      history.push(WalletRoutes.BackupExplainRecoveryPhrase)
    }
  }

  const handlePasswordKeyDown = (
    event: React.KeyboardEvent<HTMLInputElement>
  ) => {
    if (event.key === 'Enter') {
      onSubmit()
    }
  }

  const onPasswordChange = function (value: string): void {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }

  // render
  return (
    <OnboardingContentLayout
      showBackButton
      title={getLocale('braveWalletEnterAPasswordToContinue')}
      subTitle={getLocale('braveWalletEnterYourPasswordToStartBackup')}
    >
      <Column
        fullWidth
        alignItems='flex-start'
        margin='0 0 194px 0'
      >
        <Row justifyContent='flex-start'>
          <InputLabel htmlFor='password'>
            {getLocale('braveWalletInputLabelPassword')}{' '}
          </InputLabel>
        </Row>
        <InputLabel></InputLabel>
        <PasswordInput
          key='password'
          placeholder={getLocale('braveWalletEnterYourPassword')}
          onChange={onPasswordChange}
          onKeyDown={handlePasswordKeyDown}
          hasError={!!password && !isCorrectPassword}
          value={password}
          error={getLocale('braveWalletLockScreenError')}
          autoFocus
          name='password'
        />
      </Column>

      <NextButtonRow>
        <ContinueButton
          onClick={onSubmit}
          isDisabled={!password || !isCorrectPassword}
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </NextButtonRow>
    </OnboardingContentLayout>
  )
}

export default BackupEnterPassword
