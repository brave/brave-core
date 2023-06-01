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

// hooks
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'

// types
import {
  BraveWallet,
  WalletAccountType,
  AccountButtonOptionsObjectType,
  AccountModalTypes
} from '../../../constants/types'

// options
import { AccountButtonOptions } from '../../../options/account-list-button-options'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'
import {
  AccountActionsMenu
} from '../wallet-menus/account-actions-menu'

// style
import {
  StyledWrapper,
  NameAndIcon,
  AccountCircle,
  AccountMenuWrapper,
  HardwareIcon,
  AccountNameRow,
  AccountMenuButton,
  AccountMenuIcon
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
}

export const AccountListItem = ({
  account,
  onClick
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // state
  const [showAccountMenu, setShowAccountMenu] = React.useState<boolean>(false)

  // refs
  const accountMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    accountMenuRef,
    () => setShowAccountMenu(false),
    showAccountMenu
  )

  // methods
  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [onClick, account])

  const onRemoveAccount = React.useCallback(() => {
    dispatch(
      AccountsTabActions.setAccountToRemove({
        accountId: account.accountId,
        name: account.name
      })
    )
  }, [account])

  const onShowAccountsModal = React.useCallback((modalType: AccountModalTypes) => {
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(modalType))
    dispatch(AccountsTabActions.setSelectedAccount(account))
  }, [account, dispatch])

  const onClickButtonOption = React.useCallback((id: AccountModalTypes) => {
    if (id === 'details') {
      onSelectAccount()
      return
    }
    if (id === 'remove') {
      onRemoveAccount()
      return
    }
    onShowAccountsModal(id)
  }, [onSelectAccount, onRemoveAccount, onShowAccountsModal])

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Derived account so we filter out this option.
    if (account.accountId.kind === BraveWallet.AccountKind.kDerived) {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (account.accountId.kind === BraveWallet.AccountKind.kHardware) {
      return AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return AccountButtonOptions
  }, [account])

  const isHardwareAccount = account.accountId.kind === BraveWallet.AccountKind.kHardware;

  // render
  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareAccount && <HardwareIcon />}
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
      <AccountMenuWrapper
        ref={accountMenuRef}
      >
        <AccountMenuButton
          onClick={() => setShowAccountMenu(prev => !prev)}
        >
          <AccountMenuIcon
            name='more-vertical'
          />
        </AccountMenuButton>
        {showAccountMenu &&
          <AccountActionsMenu
            onClick={onClickButtonOption}
            options={buttonOptions}
          />
        }
      </AccountMenuWrapper>
    </StyledWrapper>
  )
}

export default AccountListItem
