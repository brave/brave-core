import * as React from 'react'

import {
  StyledWrapper,
  Title,
  IconBackground,
  PageIcon,
  InputColumn,
  Input
} from './style'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onConfirmPasswordChanged: (value: string) => void
  disabled: boolean
}

function OnboardingCreatePassword (props: Props) {
  const { onSubmit, onPasswordChanged, onConfirmPasswordChanged, disabled } = props

  const inputPassword = (event: React.ChangeEvent<HTMLInputElement>) => {
    onPasswordChanged(event.target.value)
  }

  const confirmPassword = (event: React.ChangeEvent<HTMLInputElement>) => {
    onConfirmPasswordChanged(event.target.value)
  }

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
      <InputColumn>
        <Input type='password' placeholder={locale.createPasswordInput} onChange={inputPassword} />
        <Input type='password' placeholder={locale.createPasswordInput2} onChange={confirmPassword} onKeyDown={handleKeyDown} />
      </InputColumn>
      <NavButton buttonType='primary' text={locale.buttonContinue} onSubmit={onSubmit} disabled={disabled} />
    </StyledWrapper>
  )
}

export default OnboardingCreatePassword
