// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'

// Types
import {
  BraveWallet,
  WalletAccountType,
  AccountButtonOptionsObjectType
} from '../../../constants/types'

// Components
import { create } from 'ethereum-blockies'
import { Tooltip } from '../../shared'
import AccountListItemButton from './account-list-item-button'
import { AccountButtonOptions } from '../../../options/account-buttons'

// Utils
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
  onDelete?: () => void
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

  // custom hooks
  const { copied, copyText } = useCopy()

  // methods
  const onCopyToClipboard = async () => {
    await copyText(account.address)
  }

  const onSelectAccount = () => {
    onClick(account)
  }

  const removeAccount = () => {
    let confirmAction = confirm(`Are you sure to remove ${account.name}?`)
    if (confirmAction) {
      onRemoveAccount(account.address, isHardwareWallet, account.coin)
    }
  }

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  const onClickButtonOption = React.useCallback((option: AccountButtonOptionsObjectType) => {
    switch (option.id) {
      case 'export':
        return
      case 'deposit':
        return
      case 'edit':
        return
      case 'details':
        onSelectAccount()
        return
      case 'remove':
        removeAccount()
    }
  }, [])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    // We are not able to remove a Primary account so we filter out this option.
    if (account.accountType === 'Primary') {
      return AccountButtonOptions.filter((option) => option.id !== 'remove')
    }
    return AccountButtonOptions
  }, [account])

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
            onClick={() => onClickButtonOption(option)}
          />
        )}
      </RightSide>
    </StyledWrapper>
  )
}

export default AccountListItem
