// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// actions
import {
  AccountsTabActions,
  AccountsTabState
} from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale } from '../../../../../common/locale'
import { BraveWallet } from '../../../../constants/types'

// hooks
import {
  usePasswordAttempts //
} from '../../../../common/hooks/use-password-attempts'
import { useRemoveAccountMutation } from '../../../../common/slices/api.slice'

// components
import { PopupModal } from '../index'
import { PasswordInput } from '../../../shared/password-input/index'

// style
import { LeoSquaredButton, Row } from '../../../shared/style'
import { Title } from '../style'
import { modalWidth, StyledWrapper } from './remove-account-modal.style'

export const RemoveAccountModal = () => {
  // redux
  const dispatch = useDispatch()

  // accounts tab state
  const accountToRemove = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.accountToRemove
  )

  // state
  const [password, setPassword] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] =
    React.useState<boolean>(true)

  // mutations
  const [removeAccount] = useRemoveAccountMutation()

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // methods
  const onConfirmRemoveAccount = React.useCallback(
    async (password: string) => {
      if (!accountToRemove) {
        return
      }

      const { accountId } = accountToRemove

      if (
        accountId.kind === BraveWallet.AccountKind.kHardware ||
        accountId.kind === BraveWallet.AccountKind.kImported
      ) {
        await removeAccount({ accountId, password })
      }

      dispatch(AccountsTabActions.setAccountToRemove(undefined)) // close modal
    },
    [accountToRemove, dispatch, removeAccount]
  )

  const onSubmit = React.useCallback(async () => {
    if (!password) {
      // require password to view key
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
  }, [attemptPasswordEntry, password, onConfirmRemoveAccount])

  const onPasswordChange = React.useCallback((value: string): void => {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }, [])

  const handlePasswordKeyDown = React.useCallback(
    (event: React.KeyboardEvent<HTMLInputElement>) => {
      if (event.key === 'Enter') {
        onSubmit()
      }
    },
    [onSubmit]
  )

  // computed
  const title = accountToRemove
    ? getLocale('braveWalletRemoveAccountModalTitle').replace(
        '$1',
        accountToRemove.name ?? accountToRemove.accountId.address
      )
    : undefined

  // render
  return (
    <PopupModal
      title=''
      onClose={() => dispatch(AccountsTabActions.setAccountToRemove(undefined))}
      width={modalWidth}
    >
      <StyledWrapper>
        {title && (
          <Row
            alignItems={'flex-start'}
            justifyContent={'flex-start'}
            marginBottom={'24px'}
          >
            <Title>{title}</Title>
          </Row>
        )}
        <Row marginBottom={'24px'}>
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

        <Row>
          <LeoSquaredButton
            onClick={onSubmit}
            kind='filled'
            isDisabled={password ? !isCorrectPassword : true}
          >
            {getLocale('braveWalletButtonContinue')}
          </LeoSquaredButton>
        </Row>
      </StyledWrapper>
    </PopupModal>
  )
}
