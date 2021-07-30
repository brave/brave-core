import * as React from 'react'

import {
  StyledWrapper,
  Title,
  IconBackground,
  PageIcon,
  InputColumn,
  Description
} from './style'
import { PasswordInput } from '../../../shared'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onConfirmPasswordChanged: (value: string) => void
  disabled: boolean
  hasPasswordError: boolean
  hasConfirmPasswordError: boolean
}

function OnboardingCreatePassword (props: Props) {
  const {
    onSubmit,
    onPasswordChanged,
    onConfirmPasswordChanged,
    disabled,
    hasConfirmPasswordError,
    hasPasswordError
  } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  return (
    <StyledWrapper>
      <IconBackground>
        <PageIcon />
      </IconBackground>
      <Title>{locale.createPasswordTitle}</Title>
      <Description>{locale.createPasswordDescription}</Description>
      <InputColumn>
        <PasswordInput
          placeholder={locale.createPasswordInput}
          onChange={onPasswordChanged}
          error={locale.createPasswordError}
          hasError={hasPasswordError}
          autoFocus={true}
        />
        <PasswordInput
          placeholder={locale.createPasswordInput2}
          onChange={onConfirmPasswordChanged}
          onKeyDown={handleKeyDown}
          error={locale.createPasswordError2}
          hasError={hasConfirmPasswordError}
        />
      </InputColumn>
      <NavButton buttonType='primary' text={locale.buttonContinue} onSubmit={onSubmit} disabled={disabled} />
    </StyledWrapper>
  )
}

export default OnboardingCreatePassword
