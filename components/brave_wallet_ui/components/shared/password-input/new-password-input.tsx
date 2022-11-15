// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// hooks
import { usePasswordStrength } from '../../../common/hooks/use-password-strength'

// components
import { PasswordStrengthBar } from './password-strength-bar'
import { PasswordInput } from './index'

// style
import {
  Column,
  VerticalSpace
} from '../style'
import {
  PasswordMatchRow,
  PasswordMatchText,
  PasswordMatchCheckmark
} from './new-password-input.styles'

export interface NewPasswordValues {
  password: string
  isValid: boolean
}

export interface Props {
  autoFocus?: boolean
  showToggleButton?: boolean
  onSubmit: (values: NewPasswordValues) => void
  onChange: (values: NewPasswordValues) => void
}

export const NewPasswordInput = ({
  autoFocus,
  showToggleButton,
  onSubmit,
  onChange
}: Props) => {
  // state
  const [showPassword, setShowPassword] = React.useState(false)
  const [isPasswordFieldFocused, setIsPasswordFieldFocused] = React.useState(false)

  // custom hooks
  const {
    confirmedPassword,
    hasConfirmedPasswordError,
    hasPasswordError,
    passwordStrength,
    isValid,
    onPasswordChanged,
    password,
    setConfirmedPassword,
    passwordsMatch
  } = usePasswordStrength()

  // methods
  const onTogglePasswordVisibility = React.useCallback(() => {
    setShowPassword(prevShowPassword => !prevShowPassword)
  }, [])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      if (!hasConfirmedPasswordError) {
        onSubmit({ isValid, password })
      }
    }
  }, [hasConfirmedPasswordError, onSubmit, isValid, password])

  // effect
  React.useEffect(() => {
    onChange({ isValid, password })
  }, [onChange, isValid, password])

  // render
  return (
    <>
      <Column fullWidth gap={'0px'}>
        <PasswordInput
          autoFocus={autoFocus}
          error={''}
          hasError={hasPasswordError}
          key={'password'}
          label={getLocale('braveWalletCreatePasswordInput')}
          name='password'
          onBlur={() => setIsPasswordFieldFocused(false)}
          onChange={onPasswordChanged}
          onFocus={() => setIsPasswordFieldFocused(true)}
          onKeyDown={handleKeyDown}
          onVisibilityToggled={onTogglePasswordVisibility}
          placeholder={getLocale('braveWalletCreatePasswordInput')}
          showToggleButton={true}
          value={password}
        >
          {({ value }) => value
            ? <PasswordStrengthBar
                criteria={[
                  passwordStrength.isLongEnough, // weak
                  password.length >= 12, // medium
                  password.length >= 16 // strong
                ]}
                isVisible={isPasswordFieldFocused}
                passwordStrength={passwordStrength}
              />
            : <VerticalSpace space={'44px'} />
          }
        </PasswordInput>

        <PasswordInput
          autoFocus={false}
          error={''}
          hasError={hasConfirmedPasswordError}
          key={'password-confirmation'}
          label={getLocale('braveWalletConfirmPasswordInput')}
          name='password-confirmation'
          onChange={setConfirmedPassword}
          onKeyDown={handleKeyDown}
          onVisibilityToggled={onTogglePasswordVisibility}
          placeholder={getLocale('braveWalletConfirmPasswordInput')}
          showToggleButton={false}
          value={confirmedPassword}
          revealValue={showPassword}
        >
          <PasswordMatchRow>
            {passwordsMatch
              ? <>
                  <PasswordMatchCheckmark />
                  <PasswordMatchText>
                    {getLocale('braveWalletPasswordMatch')}
                  </PasswordMatchText>
                </>
              : <VerticalSpace space='44px' />
            }
          </PasswordMatchRow>
        </PasswordInput>
      </Column>
      <VerticalSpace space={'30px'} />
    </>
  )
}

NewPasswordInput.defaultProps = {
  showToggleButton: true
}

export default NewPasswordInput
