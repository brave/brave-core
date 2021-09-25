import * as React from 'react'

import {
  StyledWrapper,
  Input,
  ErrorText,
  ErrorRow,
  WarningIcon
} from './style'

export interface Props {
  onChange: (value: string) => void
  onKeyDown?: (event: React.KeyboardEvent<HTMLInputElement>) => void
  autoFocus?: boolean
  value?: string
  placeholder: string
  hasError: boolean
  error: string
}

function OnboardingCreatePassword (props: Props) {
  const { onChange, onKeyDown, placeholder, error, hasError, autoFocus, value } = props

  const inputPassword = (event: React.ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.value)
  }

  return (
    <StyledWrapper>
      <Input
        hasError={hasError}
        type='password'
        placeholder={placeholder}
        value={value}
        onChange={inputPassword}
        onKeyDown={onKeyDown}
        autoFocus={autoFocus}
      />
      {hasError &&
        <ErrorRow>
          <WarningIcon />
          <ErrorText>{error}</ErrorText>
        </ErrorRow>
      }
    </StyledWrapper>
  )
}

export default OnboardingCreatePassword
