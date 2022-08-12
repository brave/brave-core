// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// utils
import { reduceAddress } from '../../../utils/reduce-address'

// types
import {
  BraveWallet,
  WalletAccountType
} from '../../../constants/types'

// components
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// style
import {
  StyledWrapper,
  AccountName,
  AccountAddress,
  AccountAndAddress,
  NameAndIcon,
  AccountCircle,
  RightSide,
  HardwareIcon,
  AccountNameRow,
  DeleteButton,
  DeleteIcon
} from './style'

export interface Props {
  onDelete?: () => void
  onClick: (account: WalletAccountType) => void
  account: WalletAccountType
  isHardwareWallet: boolean
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
}

export const AccountListItem = ({
  account,
  isHardwareWallet,
  onClick,
  onRemoveAccount
}: Props) => {
  // methods
  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [onClick, account])

  const removeAccount = React.useCallback(() => {
    let confirmAction = confirm(`Are you sure to remove ${account.name}?`)
    if (confirmAction) {
      onRemoveAccount(account.address, isHardwareWallet, account.coin)
    }
  }, [account, isHardwareWallet, onRemoveAccount])

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  // render
  return (
    <StyledWrapper>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountAndAddress>
          <AccountNameRow>
            {isHardwareWallet && <HardwareIcon />}
            <AccountName onClick={onSelectAccount}>{account.name}</AccountName>
          </AccountNameRow>
          <CopyTooltip text={account.address}>
            <AccountAddress>{reduceAddress(account.address)}</AccountAddress>
          </CopyTooltip>
        </AccountAndAddress>
      </NameAndIcon>
      <RightSide>
        {(account.accountType !== 'Primary') &&
          <DeleteButton onClick={removeAccount}>
            <DeleteIcon />
          </DeleteButton>
        }
      </RightSide>
    </StyledWrapper>
  )
}

export default AccountListItem
