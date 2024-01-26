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
import { PasswordInput } from './password-input-v2'

// style
import { Row } from '../../shared/style'
import { Column, VerticalSpace } from '../style'
import {
  PasswordMatchRow,
  PasswordMatchText,
  PasswordMatchCheckmark,
  TooltipWrapper
} from './new-password-input.styles'
import { Asterisk, InputLabel } from './password-input-v2.style'
import PasswordStrengthTooltip from '../tooltip/password-strength-tooltip'

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
  const [isPasswordFieldFocused, setIsPasswordFieldFocused] =
    React.useState(false)

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

  const handleKeyDown = React.useCallback(
    (event: React.KeyboardEvent<HTMLInputElement>) => {
      if (event.key === 'Enter') {
        if (!hasConfirmedPasswordError) {
          onSubmit({ isValid, password })
        }
      }
    },
    [hasConfirmedPasswordError, onSubmit, isValid, password]
  )

  // effect
  React.useEffect(() => {
    onChange({ isValid, password })
  }, [onChange, isValid, password])

  // render
  return (
    <>
      <Column
        fullWidth
        gap={'0px'}
      >
        <Column
          fullWidth
          alignItems='flex-start'
        >
          <Row justifyContent='flex-start'>
            <InputLabel htmlFor='password'>
              {getLocale('braveWalletCreatePasswordInput')}
            </InputLabel>
            <Asterisk>*</Asterisk>
          </Row>
          <TooltipWrapper>
            <PasswordStrengthTooltip
              isVisible={isPasswordFieldFocused}
              passwordStrength={passwordStrength}
            />
          </TooltipWrapper>
          <PasswordInput
            autoFocus={autoFocus}
            error={''}
            hasError={hasPasswordError}
            key={'password'}
            name='password'
            onBlur={() => setIsPasswordFieldFocused(false)}
            onChange={onPasswordChanged}
            onFocus={() => setIsPasswordFieldFocused(true)}
            onKeyDown={handleKeyDown}
            placeholder={getLocale('braveWalletCreatePasswordInput')}
            showToggleButton={true}
            value={password}
          >
            {({ value }) =>
              value ? (
                <PasswordStrengthBar
                  criteria={[
                    passwordStrength.isLongEnough, // weak
                    password.length >= 12, // medium
                    password.length >= 16 // strong
                  ]}
                />
              ) : (
                <VerticalSpace space={'44px'} />
              )
            }
          </PasswordInput>
        </Column>
        <Column
          fullWidth
          alignItems='flex-start'
        >
          <Row justifyContent='flex-start'>
            <InputLabel htmlFor='password-confirmation'>
              {getLocale('braveWalletConfirmPasswordInput')}
            </InputLabel>
            <Asterisk>*</Asterisk>
          </Row>
          <VerticalSpace space='4px' />
          <PasswordInput
            autoFocus={false}
            error={''}
            hasError={hasConfirmedPasswordError}
            key={'password-confirmation'}
            name='password-confirmation'
            onChange={setConfirmedPassword}
            onKeyDown={handleKeyDown}
            placeholder={getLocale('braveWalletConfirmPasswordInput')}
            showToggleButton={true}
            value={confirmedPassword}
          >
            <PasswordMatchRow>
              {passwordsMatch ? (
                <>
                  <PasswordMatchCheckmark />
                  <PasswordMatchText>
                    {getLocale('braveWalletPasswordMatch')}
                  </PasswordMatchText>
                </>
              ) : (
                <VerticalSpace space='44px' />
              )}
            </PasswordMatchRow>
          </PasswordInput>
        </Column>
      </Column>
    </>
  )
}

NewPasswordInput.defaultProps = {
  showToggleButton: true
}

export default NewPasswordInput
