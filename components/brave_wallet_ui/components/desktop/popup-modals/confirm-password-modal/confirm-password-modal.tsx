// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// actions
import { WalletPageActions } from '../../../../page/actions'
import { AccountsTabActions, AccountsTabState } from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale } from '../../../../../common/locale'

// hooks
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'

// components
import { PopupModal } from '../..'
import { NavButton } from '../../../extension'
import { PasswordInput } from '../../../shared'

// style
import {
  Row,
  VerticalSpace
} from '../../../shared/style'
import { Title } from '../style'
import {
  modalWidth,
  StyledWrapper
} from './confirm-password-modal.style'

export const ConfirmPasswordModal = () => {
  // redux
  const dispatch = useDispatch()

  // accounts tab state
  const accountToRemove = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.accountToRemove)

  // state
  const [password, setPassword] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] = React.useState<boolean>(true)

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // methods
  const onConfirmRemoveAccount = React.useCallback((password: string) => {
    if (!accountToRemove) {
      return
    }

    const { address, coin, hardware } = accountToRemove

    if (hardware) {
      dispatch(WalletPageActions.removeHardwareAccount({ address, coin }))
    }

    if (!hardware) {
      dispatch(WalletPageActions.removeImportedAccount({ address, coin, password }))
    }

    dispatch(AccountsTabActions.setAccountToRemove(undefined)) // close modal
  }, [accountToRemove, dispatch])

  const onSubmit = React.useCallback(async () => {
    if (!password) { // require password to view key
      return
    }

    // entered password must be correct
    const isPasswordValid = await attemptPasswordEntry(password)

    if (!isPasswordValid) {
      setIsCorrectPassword(isPasswordValid) // set or clear error
      return // need valid password to continue
    }

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    onConfirmRemoveAccount(password)
  }, [
    attemptPasswordEntry,
    password,
    onConfirmRemoveAccount
  ])

  const onPasswordChange = React.useCallback((value: string): void => {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }, [])

  const handlePasswordKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onSubmit()
    }
  }, [onSubmit])

  // memos
  const title = React.useMemo(() => {
    if (!accountToRemove) {
      return
    }
    return getLocale('braveWalletRemoveAccountModalTitle')
      .replace('$1', accountToRemove.name ?? accountToRemove.address)
  }, [accountToRemove])

  // render
  return (
    <PopupModal
      title=''
      onClose={() => dispatch(AccountsTabActions.setAccountToRemove(undefined))}
      width={modalWidth}
    >
      <StyledWrapper>

        {title &&
          <>
            <Row alignItems={'flex-start'} justifyContent={'flex-start'}>
              <Title>{title}</Title>
            </Row>
            <VerticalSpace space='24px' />
          </>
        }

        <Row>
          <PasswordInput
            placeholder={getLocale('braveWalletEnterYourPassword')}
            onChange={onPasswordChange}
            hasError={!!password && !isCorrectPassword}
            error={getLocale('braveWalletLockScreenError')}
            autoFocus={false}
            value={password}
            onKeyDown={handlePasswordKeyDown}
          />
        </Row>

        <VerticalSpace space='24px' />

        <Row>
          <NavButton
            onSubmit={onSubmit}
            text={getLocale('braveWalletButtonContinue')}
            buttonType='primary'
            disabled={password ? !isCorrectPassword : true}
          />
        </Row>
      </StyledWrapper>
    </PopupModal>
  )
}
