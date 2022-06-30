// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Redux
import { useDispatch } from 'react-redux'

// Actions
import { AccountsTabActions } from '../../../page/reducers/accounts-tab-reducer'

// Types
import {
  BraveWallet,
  WalletAccountType,
  AccountButtonOptionsObjectType,
  AccountModalTypes
} from '../../../constants/types'

// Components
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import AccountListItemButton from './account-list-item-button'
import { AccountButtonOptions } from '../../../options/account-buttons'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'
import { useCopy } from '../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  NameAndIcon,
  AccountCircle,
  RightSide,
  HardwareIcon,
  AccountNameRow
} from './style'

export interface Props {
  onClick: (account: WalletAccountType) => void
  account: WalletAccountType
  isHardwareWallet: boolean
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
}

function AccountListItem (props: Props) {
  const {
    account,
    isHardwareWallet,
    onClick,
    onRemoveAccount
  } = props

  // redux
  const dispatch = useDispatch()

  // custom hooks
  const { copied, copyText } = useCopy()

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Primary account so we filter out this option.
    if (account.accountType === 'Primary') {
      return AccountButtonOptions.filter((option) => option.id !== 'remove')
    }
    return AccountButtonOptions
  }, [account])

  // methods
  const onCopyToClipboard = async () => {
    await copyText(account.address)
  }

  const onSelectAccount = () => {
    onClick(account)
  }

  const removeAccount = React.useCallback(() => {
    let confirmAction = confirm(`${getLocale('braveWalletAccountsRemoveMessage').replace('$1', account.name)}`)
    if (confirmAction) {
      onRemoveAccount(account.address, isHardwareWallet, account.coin)
    }
  }, [account, onRemoveAccount])

  const onShowAccountsModal = React.useCallback((modalType: AccountModalTypes) => {
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(modalType))
    dispatch(AccountsTabActions.setSelectedAccount(account))
  }, [account])

  const onClickButtonOption = React.useCallback((option: AccountButtonOptionsObjectType) => () => {
    if (option.id === 'details') {
      onSelectAccount()
      return
    }
    if (option.id === 'remove') {
      removeAccount()
      return
    }
    onShowAccountsModal(option.id)
  }, [])

  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareWallet && <HardwareIcon />}
            <AccountName onClick={onSelectAccount}>{account.name}</AccountName>
          </AccountNameRow>
          <Tooltip
            text={getLocale('braveWalletToolTipCopyToClipboard')}
            actionText={getLocale('braveWalletToolTipCopiedToClipboard')}
            isActionVisible={copied}
          >
            <AccountAddress onClick={onCopyToClipboard}>{reduceAddress(account.address)}</AccountAddress>
          </Tooltip>
        </AccountAndAddress>
      </NameAndIcon>
      <RightSide>
        {buttonOptions.map((option: AccountButtonOptionsObjectType) =>
          <AccountListItemButton
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
