// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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

interface PasswordInputState {
  hasError: boolean
  error: string
  showPassword: boolean
  value?: string
}

export interface Props extends Pick<React.DOMAttributes<HTMLInputElement>, 'onFocus' | 'onBlur'> {
  onChange: (value: string) => void
  onKeyDown?: (event: React.KeyboardEvent<HTMLInputElement>) => void
  onVisibilityToggled?: (isVisible: boolean) => void
  autoFocus?: boolean
  value?: string
  placeholder: string
  hasError: boolean
  error: string
  showToggleButton?: boolean
  label?: string
  name?: string
  children?: React.ReactChild | ((state: PasswordInputState) => React.ReactElement)
  revealValue?: boolean
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
  name,
  onBlur,
  onFocus,
  onVisibilityToggled,
  children,
  revealValue
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

  // effects
  React.useEffect(() => {
    onVisibilityToggled?.(showPassword) // expose state to parent
  }, [onVisibilityToggled, showPassword])

  // render
  return (
    <StyledWrapper>

      {label && name && <label htmlFor={name}>{label}</label>}

      <InputWrapper>
        <Input
          name={name}
          hasError={hasError}
          type={(revealValue || (showToggleButton && showPassword)) ? 'text' : 'password'}
          placeholder={placeholder}
          value={value}
          onChange={inputPassword}
          onKeyDown={onKeyDown}
          autoFocus={autoFocus}
          autoComplete='off'
          onBlur={onBlur}
          onFocus={onFocus}
        />
        {showToggleButton &&
          <ToggleVisibilityButton onClick={onTogglePasswordVisibility}>
            <ToggleVisibilityIcon showPassword={showPassword} />
          </ToggleVisibilityButton>
        }
      </InputWrapper>
      {hasError && error &&
        <ErrorRow>
          <WarningIcon />
          <ErrorText>{error}</ErrorText>
        </ErrorRow>
      }
      {/* Allow custom child elements */}
      {children &&
        typeof children === 'function'
          ? children({
              error,
              hasError,
              showPassword,
              value
            })
          : children
      }
    </StyledWrapper>
  )
}

PasswordInput.defaultProps = {
  showToggleButton: true
}

export default PasswordInput
