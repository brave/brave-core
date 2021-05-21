import * as React from 'react'

import {
  StyledWrapper,
  Title,
  IconBackground,
  PageIcon,
  InputColumn,
  Input
} from '../wallet-onboarding/create-password/style'
import { NavButton } from '../../extension'
import locale from '../../../constants/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  disabled: boolean
}

function LockScreen (props: Props) {
  const { onSubmit, onPasswordChanged, disabled } = props

  const inputPassword = (event: React.ChangeEvent<HTMLInputElement>) => {
    onPasswordChanged(event.target.value)
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
      <Title>{locale.lockScreenTitle}</Title>
      <InputColumn>
        <Input
          type='password'
          placeholder={locale.createPasswordInput}
          onChange={inputPassword}
          onKeyDown={handleKeyDown}
          autoFocus={true}
        />
      </InputColumn>
      <NavButton
        buttonType='primary'
        text={locale.lockScreenButton}
        onSubmit={onSubmit}
        disabled={disabled}
      />
    </StyledWrapper>
  )
}

export default LockScreen
