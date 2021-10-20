import * as React from 'react'

import {
  StyledWrapper,
  Title,
  Column,
  PanelIcon,
  RestoreButton
} from './style'
import { PasswordInput } from '../../shared'
import { NavButton } from '../'
import { getLocale } from '../../../../common/locale'

export interface Props {
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onClickRestore: () => void
  hasPasswordError: boolean
  disabled: boolean
}

function LockPanel (props: Props) {
  const { onSubmit, onPasswordChanged, onClickRestore, disabled, hasPasswordError } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  return (
    <StyledWrapper>
      <PanelIcon />
      <Title>{getLocale('braveWalletLockScreenTitle')}</Title>
      <Column>
        <PasswordInput
          placeholder={getLocale('braveWalletCreatePasswordInput')}
          onChange={onPasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletLockScreenError')}
          hasError={hasPasswordError}
          autoFocus={true}
        />
      </Column>
      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletLockScreenButton')}
        onSubmit={onSubmit}
        disabled={disabled}
      />
      <RestoreButton onClick={onClickRestore}>{getLocale('braveWalletWelcomeRestoreButton')}</RestoreButton>
    </StyledWrapper>
  )
}

export default LockPanel
