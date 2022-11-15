// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// redux
import { useDispatch } from 'react-redux'

// actions
import { AccountsTabActions } from '../../../page/reducers/accounts-tab-reducer'

// utils
import { reduceAddress } from '../../../utils/reduce-address'

// types
import {
  WalletAccountType,
  AccountButtonOptionsObjectType,
  AccountModalTypes
} from '../../../constants/types'

// options
import { AccountButtonOptions } from '../../../options/account-list-button-options'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import { AccountListItemOptionButton } from './account-list-item-option-button'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountCircle,
  RightSide,
  HardwareIcon,
  AccountNameRow
} from './style'

import {
  AccountAddressButton,
  AccountAndAddress,
  AccountNameButton,
  AddressAndButtonRow,
  CopyIcon
} from '../portfolio-account-item/style'

export interface Props {
  onDelete?: () => void
  onClick: (account: WalletAccountType) => void
  account: WalletAccountType
  isHardwareWallet: boolean
}

export const AccountListItem = ({
  account,
  isHardwareWallet,
  onClick
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // methods
  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [onClick, account])

  const onRemoveAccount = React.useCallback(() => {
    dispatch(AccountsTabActions.setAccountToRemove({ address: account.address, hardware: isHardwareWallet, coin: account.coin, name: account.name }))
  }, [account, isHardwareWallet])

  const onShowAccountsModal = React.useCallback((modalType: AccountModalTypes) => {
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(modalType))
    dispatch(AccountsTabActions.setSelectedAccount(account))
  }, [account, dispatch])

  const onClickButtonOption = React.useCallback((option: AccountButtonOptionsObjectType) => () => {
    if (option.id === 'details') {
      onSelectAccount()
      return
    }
    if (option.id === 'remove') {
      onRemoveAccount()
      return
    }
    onShowAccountsModal(option.id)
  }, [onSelectAccount, onRemoveAccount, onShowAccountsModal])

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Primary account so we filter out this option.
    if (account.accountType === 'Primary') {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (isHardwareWallet) {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return AccountButtonOptions
  }, [account, isHardwareWallet])

  // render
  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareWallet && <HardwareIcon />}
            <AccountNameButton onClick={onSelectAccount}>{account.name}</AccountNameButton>
          </AccountNameRow>
          <AddressAndButtonRow>
            <AccountAddressButton onClick={onSelectAccount}>{reduceAddress(account.address)}</AccountAddressButton>
            <CopyTooltip text={account.address}>
              <CopyIcon />
            </CopyTooltip>
          </AddressAndButtonRow>
        </AccountAndAddress>
      </NameAndIcon>
      <RightSide>
        {buttonOptions.map((option: AccountButtonOptionsObjectType) =>
          <AccountListItemOptionButton
            key={option.id}
            option={option}
            onClick={onClickButtonOption(option)}
          />
        )}
      </RightSide>
    </StyledWrapper>
  )
}

export default AccountListItem
