import * as React from 'react'

import {
  StyledWrapper,
  Title,
  PageIcon,
  InputColumn,
  RestoreButton
} from './style'
import { PasswordInput } from '../../shared'
import { NavButton } from '../../extension'
import { getLocale } from '../../../../common/locale'

export interface Props {
  value?: string
  onSubmit: () => void
  onPasswordChanged: (value: string) => void
  onShowRestore: () => void
  hasPasswordError: boolean
  disabled: boolean
}

function LockScreen (props: Props) {
  const { value, onSubmit, onPasswordChanged, onShowRestore, disabled, hasPasswordError } = props

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit()
    }
  }

  return (
    <StyledWrapper>
      <PageIcon />
      <Title>{getLocale('braveWalletLockScreenTitle')}</Title>
      <InputColumn>
        <PasswordInput
          placeholder={getLocale('braveWalletCreatePasswordInput')}
          onChange={onPasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletLockScreenError')}
          hasError={hasPasswordError}
          autoFocus={true}
          value={value}
        />
      </InputColumn>
      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletLockScreenButton')}
        onSubmit={onSubmit}
        disabled={disabled}
      />
      <RestoreButton onClick={onShowRestore}>{getLocale('braveWalletWelcomeRestoreButton')}</RestoreButton>
    </StyledWrapper>
  )
}

export default LockScreen
