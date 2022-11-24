// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// utils
import { getLocale } from '../../../../common/locale'

// actions
import { WalletActions } from '../../../common/actions'

// types
import { WalletState } from '../../../constants/types'

// components
import { PasswordInput } from '../../shared'
import { NavButton } from '../'

// style
import {
  StyledWrapper,
  Title,
  Column,
  PanelIcon,
  RestoreButton
} from './style'

export interface Props {
  onSubmit: (password: string) => void
  onClickRestore?: () => void
  disabled?: boolean
  hideBackground?: boolean
}

export function LockPanel ({
  onSubmit,
  onClickRestore,
  disabled,
  hideBackground
}: Props) {
  // redux
  const dispatch = useDispatch()
  const hasIncorrectPassword = useSelector(({
    wallet
  }: { wallet: WalletState }) => wallet.hasIncorrectPassword)

  // state
  const [inputValue, setInputValue] = React.useState<string>('')

  // methods
  const handlePasswordChanged = React.useCallback((value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      dispatch(WalletActions.hasIncorrectPassword(false))
    }
  }, [hasIncorrectPassword])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && !disabled) {
      onSubmit(inputValue)
    }
  }, [disabled, onSubmit, inputValue])

  const onClickUnlockButton = React.useCallback(() => {
    onSubmit(inputValue)
    setInputValue('')
  }, [onSubmit, inputValue])

  // render
  return (
    <StyledWrapper hideBackground={hideBackground}>
      <PanelIcon />
      <Title>{getLocale('braveWalletLockScreenTitle')}</Title>
      <Column>
        <PasswordInput
          placeholder={getLocale('braveWalletEnterYourPassword')}
          onChange={handlePasswordChanged}
          onKeyDown={handleKeyDown}
          error={getLocale('braveWalletLockScreenError')}
          hasError={hasIncorrectPassword}
          autoFocus={true}
          value={inputValue}
        />
      </Column>
      <NavButton
        buttonType='primary'
        text={getLocale('braveWalletLockScreenButton')}
        onSubmit={onClickUnlockButton}
        disabled={disabled || inputValue === ''}
      />
      {onClickRestore && <RestoreButton onClick={onClickRestore}>
        {getLocale('braveWalletWelcomeRestoreButton')}
      </RestoreButton>}
    </StyledWrapper>
  )
}

export default LockPanel
