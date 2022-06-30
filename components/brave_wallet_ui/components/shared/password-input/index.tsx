// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// style
import {
  StyledWrapper,
  InputWrapper,
  ToggleVisibilityButton,
  ToggleVisibilityIcon,
  Input,
  ErrorText,
  ErrorRow,
  WarningIcon
} from './style'

export interface Props extends Partial<Pick<HTMLInputElement, 'name'>> {
  onChange: (value: string) => void
  onKeyDown?: (event: React.KeyboardEvent<HTMLInputElement>) => void
  autoFocus?: boolean
  value?: string
  placeholder: string
  hasError: boolean
  error: string
  showToggleButton?: boolean
  label?: string
  labelName?: string
}

export function PasswordInput ({
  onChange,
  onKeyDown,
  placeholder,
  error,
  hasError,
  autoFocus,
  value,
  showToggleButton = true,
  label,
  name
}: Props) {
  // state
  const [showPassword, setShowPassword] = React.useState(false)

  // methods
  const inputPassword = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.value)
  }, [onChange])

  const onTogglePasswordVisibility = React.useCallback(() => {
    setShowPassword(prev => !prev)
  }, [])

  // render
  return (
    <StyledWrapper>

      {label && name && <label htmlFor={name}>{label}</label>}

      <InputWrapper>
        <Input
          name={name}
          hasError={hasError}
          type={(showToggleButton && showPassword) ? 'text' : 'password'}
          placeholder={placeholder}
          value={value}
          onChange={inputPassword}
          onKeyDown={onKeyDown}
          autoFocus={autoFocus}
          autoComplete='off'
        />
        {showToggleButton &&
          <ToggleVisibilityButton onClick={onTogglePasswordVisibility}>
            <ToggleVisibilityIcon showPassword={showPassword} />
          </ToggleVisibilityButton>
        }
      </InputWrapper>
      {hasError &&
        <ErrorRow>
          <WarningIcon />
          <ErrorText>{error}</ErrorText>
        </ErrorRow>
      }
    </StyledWrapper>
  )
}

PasswordInput.defaultProps = {
  showToggleButton: true
}

export default PasswordInput
