// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// hooks
import { usePasswordStrength } from '../../../common/hooks/use-password-strength'

// components
import { PasswordStrengthBar } from './password-strength-bar'

// style
import {
  Column,
  ToggleVisibilityButton,
  VerticalSpace
} from '../style'
import {
  StyledWrapper,
  InputWrapper,
  Input,
  InputLabel,
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
    isStrongPassword,
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

  const handlePasswordChanged = React.useCallback(async (event: React.ChangeEvent<HTMLInputElement>) => {
    const newPassword = event.target.value
    onPasswordChanged(newPassword)
    onChange({
      isValid: confirmedPassword === newPassword && isStrongPassword,
      password: newPassword
    })
  }, [onPasswordChanged, onChange, confirmedPassword, isStrongPassword])

  const handlePasswordConfirmationChanged = React.useCallback(async (event: React.ChangeEvent<HTMLInputElement>) => {
    setConfirmedPassword(event.target.value)
  }, [])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      if (!hasConfirmedPasswordError) {
        onSubmit({
          isValid: confirmedPassword === password && isStrongPassword,
          password
        })
      }
    }
  }, [hasConfirmedPasswordError, onSubmit, confirmedPassword, password, isStrongPassword])

  // effect
  React.useEffect(() => {
    onChange({ isValid, password })
  }, [onChange, isValid, password])

  // render
  return (
    <>
      <Column fullWidth gap={'0px'}>
        <StyledWrapper>
          <InputLabel>{getLocale('braveWalletCreatePasswordInput')}</InputLabel>

          <InputWrapper>
            <Input
              hasError={!!hasPasswordError}
              type={(showToggleButton && showPassword) ? 'text' : 'password'}
              placeholder={getLocale('braveWalletCreatePasswordInput')}
              value={password}
              onChange={handlePasswordChanged}
              onKeyDown={handleKeyDown}
              autoFocus={autoFocus}
              onFocus={() => setIsPasswordFieldFocused(true)}
              onBlur={() => setIsPasswordFieldFocused(false)}
              autoComplete='off'
            />
            {showToggleButton &&
              <ToggleVisibilityButton
                isVisible={showPassword}
                onClick={onTogglePasswordVisibility}
              />
            }
          </InputWrapper>

          {password
            ? <PasswordStrengthBar
                isVisible={isPasswordFieldFocused}
                passwordStrength={passwordStrength}
                criteria={[
                  passwordStrength.containsNumber,
                  passwordStrength.containsSpecialChar,
                  passwordStrength.isLongEnough
                ]}
              />
            : <VerticalSpace space={'44px'} />
          }

        </StyledWrapper>

        <StyledWrapper>
          <InputLabel>{getLocale('braveWalletConfirmPasswordInput')}</InputLabel>
          <InputWrapper>
            <Input
              hasError={!!hasConfirmedPasswordError}
              type={(showToggleButton && showPassword) ? 'text' : 'password'}
              placeholder={getLocale('braveWalletConfirmPasswordInput')}
              value={confirmedPassword}
              onChange={handlePasswordConfirmationChanged}
              onKeyDown={handleKeyDown}
              autoFocus={false}
              autoComplete='off'
            />
            {showToggleButton &&
              <ToggleVisibilityButton
                isVisible={showPassword}
                onClick={onTogglePasswordVisibility}
              />
            }
          </InputWrapper>

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

        </StyledWrapper>
      </Column>
      <VerticalSpace space={'30px'} />
    </>
  )
}

NewPasswordInput.defaultProps = {
  showToggleButton: true
}

export default NewPasswordInput
