import * as React from 'react'

import {
  StyledWrapper,
  Title,
  IconBackground,
  PageIcon,
  InputColumn,
  RestoreButton
} from './style'
import { PasswordInput } from '../../shared'
import { NavButton } from '../../extension'
import locale from '../../../constants/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onShowRestore: () => void
  hasPasswordError: boolean
  disabled: boolean
}

function LockScreen (props: Props) {
  const { onSubmit, onPasswordChanged, onShowRestore, disabled, hasPasswordError } = props

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
        <PasswordInput
          placeholder={locale.createPasswordInput}
          onChange={onPasswordChanged}
          onKeyDown={handleKeyDown}
          error={locale.lockScreenError}
          hasError={hasPasswordError}
          autoFocus={true}
        />
      </InputColumn>
      <NavButton
        buttonType='primary'
        text={locale.lockScreenButton}
        onSubmit={onSubmit}
        disabled={disabled}
      />
      <RestoreButton onClick={onShowRestore}>{locale.welcomeRestoreButton}</RestoreButton>
    </StyledWrapper>
  )
}

export default LockScreen
