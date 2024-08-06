// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import * as leo from '@brave/leo/tokens/css/variables'
import { type InputEventDetail } from '@brave/leo/react/input'

// actions
import {
  AccountsTabActions,
  AccountsTabState
} from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { loadTimeData } from '../../../../../common/loadTimeData'
import { getLocale } from '../../../../../common/locale'
import { BraveWallet } from '../../../../constants/types'
import { UISelectors } from '../../../../common/selectors'

// hooks
import {
  usePasswordAttempts //
} from '../../../../common/hooks/use-password-attempts'
import { useRemoveAccountMutation } from '../../../../common/slices/api.slice'
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'

// components
import { PopupModal } from '../index'

// style
import { Column, LeoSquaredButton, Row, Text } from '../../../shared/style'
import { modalWidth, StyledWrapper } from './remove-account-modal.style'
import {
  PasswordInputNala //
} from '../../../shared/password-input/password_input_nala'

export const RemoveAccountModal = () => {
  // redux
  const dispatch = useDispatch()

  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = loadTimeData.getBoolean('isAndroid') || false

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

  const onPasswordChange = React.useCallback(
    (detail: InputEventDetail): void => {
      setIsCorrectPassword(true) // clear error
      setPassword(detail.value)
    },
    []
  )

  const handlePasswordKeyDown = React.useCallback(
    (detail: InputEventDetail) => {
      const key = (detail.innerEvent as unknown as KeyboardEvent).key
      if (key === 'Enter') {
        onSubmit()
      }
    },
    [onSubmit]
  )

  // render
  return (
    <PopupModal
      title={getLocale('braveWalletAccountSettingsRemove')}
      onClose={() => dispatch(AccountsTabActions.setAccountToRemove(undefined))}
      width={modalWidth}
      headerPaddingHorizontal={leo.spacing['3Xl']}
    >
      <StyledWrapper>
        <Column
          fullWidth
          alignItems={'flex-start'}
        >
          {accountToRemove && (
            <Column
              alignItems={'flex-start'}
              justifyContent={'flex-start'}
              margin={`0px 0px ${leo.spacing['2Xl']} 0px`}
              gap={leo.spacing.m}
            >
              <Text
                textSize='16px'
                textAlign='left'
              >
                {getLocale('braveWalletRemoveAccountModalTitle').replace(
                  '$1',
                  accountToRemove.name ?? accountToRemove.accountId.address
                )}
              </Text>

              <Text
                textSize='16px'
                textColor={'tertiary'}
              >
                {getLocale('braveWalletPasswordIsRequiredToTakeThisAction')}
              </Text>
            </Column>
          )}

          <Row
            marginBottom={'24px'}
            alignItems='center'
            justifyContent='stretch'
          >
            <PasswordInputNala
              onChange={onPasswordChange}
              onKeyDown={handlePasswordKeyDown}
              password={password}
              isCorrectPassword={isCorrectPassword}
            />
          </Row>
        </Column>

        {isMobile || isPanel ? (
          <Row gap='4px'>
            <LeoSquaredButton
              onClick={() =>
                dispatch(AccountsTabActions.setAccountToRemove(undefined))
              }
              kind='plain-faint'
            >
              {getLocale('braveWalletButtonCancel')}
            </LeoSquaredButton>

            <LeoSquaredButton
              onClick={onSubmit}
              kind='filled'
              isDisabled={password ? !isCorrectPassword : true}
            >
              {getLocale('braveWalletAccountsRemove')}
            </LeoSquaredButton>
          </Row>
        ) : (
          <Row
            gap='4px'
            justifyContent={'flex-end'}
          >
            <div>
              <LeoSquaredButton
                onClick={() =>
                  dispatch(AccountsTabActions.setAccountToRemove(undefined))
                }
                kind='plain-faint'
              >
                {getLocale('braveWalletButtonCancel')}
              </LeoSquaredButton>
            </div>

            <div>
              <LeoSquaredButton
                onClick={onSubmit}
                kind='filled'
                isDisabled={password ? !isCorrectPassword : true}
              >
                {getLocale('braveWalletAccountsRemove')}
              </LeoSquaredButton>
            </div>
          </Row>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}
