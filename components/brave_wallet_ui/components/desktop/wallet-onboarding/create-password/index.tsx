// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'

// actions
import { WalletPageActions } from '../../../../page/actions'

// constants
import { WalletRoutes } from '../../../../constants/types'

// hooks
import { usePasswordStrength } from '../../../../common/hooks/use-password-strength'

// components
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'

// style
import {
  StyledWrapper,
  Title,
  IconBackground,
  PageIcon,
  InputColumn,
  Description
} from './style'

export const OnboardingCreatePassword = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // custom hooks
  const {
    onPasswordChanged,
    password,
    hasConfirmedPasswordError,
    hasPasswordError,
    confirmedPassword,
    setConfirmedPassword
  } = usePasswordStrength()

  // computed
  const disabled = hasConfirmedPasswordError ||
    hasPasswordError ||
    password === '' ||
    confirmedPassword === ''

  // methods
  const onSubmit = React.useCallback(() => {
    dispatch(WalletPageActions.createWallet({ password }))
    history.push(WalletRoutes.OnboardingBackupWallet)
  }, [password])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }, [onSubmit, disabled])

  // render
  return (
    <StyledWrapper>

      <IconBackground>
        <PageIcon />
      </IconBackground>

      <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
      <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>

      <InputColumn>
        <PasswordInput
          placeholder={getLocale('braveWalletCreatePasswordInput')}
          onChange={onPasswordChanged}
          error={getLocale('braveWalletCreatePasswordError')}
          onKeyDown={handleKeyDown}
          hasError={hasPasswordError}
          autoFocus={true}
        />
        <PasswordInput
          placeholder={getLocale('braveWalletConfirmPasswordInput')}
          onChange={setConfirmedPassword}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletConfirmPasswordError')}
          hasError={hasConfirmedPasswordError}
        />
      </InputColumn>

      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletButtonContinue')}
        onSubmit={onSubmit}
        disabled={disabled}
      />

    </StyledWrapper>
  )
}

export default OnboardingCreatePassword
