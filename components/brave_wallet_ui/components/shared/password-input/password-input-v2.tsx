// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  ToggleVisibilityButton,
  ToggleVisibilityIcon,
  Input,
  ErrorText,
  ErrorIcon
} from './password-input-v2.style'
import { Column, Row } from '../style'

interface PasswordInputState {
  hasError: boolean
  error: string
  showPassword: boolean
  value?: string
}

export interface Props
  extends Pick<React.DOMAttributes<HTMLInputElement>, 'onFocus' | 'onBlur'> {
  onChange: (value: string) => void
  onKeyDown?: (event: React.KeyboardEvent<HTMLInputElement>) => void
  autoFocus?: boolean
  value?: string
  placeholder: string
  hasError: boolean
  error: string
  showToggleButton?: boolean
  name?: string
  children?:
    | React.ReactChild
    | ((state: PasswordInputState) => React.ReactElement)
  revealValue?: boolean
}

export const PasswordInput = (props: Props) => {
  const {
    onChange,
    onKeyDown,
    placeholder,
    error,
    hasError,
    autoFocus,
    value,
    showToggleButton = true,
    name,
    onBlur,
    onFocus,
    children,
    revealValue
  } = props

  // state
  const [showPassword, setShowPassword] = React.useState(false)

  // methods
  const inputPassword = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  // render
  return (
    <Column
      alignItems='flex-start'
      justifyContent='flex-start'
      fullWidth={true}
    >
      <Row
        justifyContent='space-between'
        alignItems='center'
      >
        <Input
          name={name}
          hasError={hasError}
          type={
            revealValue || (showToggleButton && showPassword)
              ? 'text'
              : 'password'
          }
          placeholder={placeholder}
          value={value}
          onChange={inputPassword}
          onKeyDown={onKeyDown}
          autoFocus={autoFocus}
          autoComplete='off'
          onBlur={onBlur}
          onFocus={onFocus}
        />
        {showToggleButton && (
          <ToggleVisibilityButton
            onClick={() => setShowPassword((prev) => !prev)}
          >
            <ToggleVisibilityIcon name={showPassword ? 'eye-off' : 'eye-on'} />
          </ToggleVisibilityButton>
        )}
      </Row>
      {hasError && error && (
        <Row
          justifyContent='flex-start'
          alignItems='center'
          margin='4px 0px 0px 0px'
          padding='2px 6px'
        >
          <ErrorIcon />
          <ErrorText textSize='12px'>{error}</ErrorText>
        </Row>
      )}
      {/* Allow custom child elements */}
      {children && typeof children === 'function'
        ? children({
            error,
            hasError,
            showPassword,
            value
          })
        : children}
    </Column>
  )
}

PasswordInput.defaultProps = {
  showToggleButton: true
}

export default PasswordInput
