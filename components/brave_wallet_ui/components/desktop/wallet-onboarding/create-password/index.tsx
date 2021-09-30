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
import { getLocale } from '../../../../../common/locale'

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
      <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
      <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>
      <InputColumn>
        <PasswordInput
          placeholder={getLocale('braveWalletCreatePasswordInput')}
          onChange={onPasswordChanged}
          error={getLocale('braveWalletCreatePasswordError')}
          hasError={hasPasswordError}
          autoFocus={true}
        />
        <PasswordInput
          placeholder={getLocale('braveWalletConfirmPasswordInput')}
          onChange={onConfirmPasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletConfirmPasswordError')}
          hasError={hasConfirmPasswordError}
        />
      </InputColumn>
      <NavButton buttonType='primary' text={getLocale('braveWalletButtonContinue')} onSubmit={onSubmit} disabled={disabled} />
    </StyledWrapper>
  )
}

export default OnboardingCreatePassword
