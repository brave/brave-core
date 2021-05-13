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

  return (
    <StyledWrapper>
      <IconBackground>
        <PageIcon />
      </IconBackground>
      <Title>{locale.lockScreenTitle}</Title>
      <InputColumn>
        <Input type='password' placeholder={locale.createPasswordInput} onChange={inputPassword} />
      </InputColumn>
      <NavButton buttonType='primary' text={locale.lockScreenButton} onSubmit={onSubmit} disabled={disabled} />
    </StyledWrapper>
  )
}

export default LockScreen
