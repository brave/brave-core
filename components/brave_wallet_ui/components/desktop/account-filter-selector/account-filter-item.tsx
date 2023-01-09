// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Options
import { AllAccountsOption } from '../../../options/account-filter-options'

// Types
import { WalletAccountType } from '../../../constants/types'

// Styled Components
import {
  AccountCircle
} from './account-filter-selector.style'

import {
  NetworkItemButton,
  NetworkName,
  NetworkItemWrapper,
  LeftSide,
  BigCheckMark
} from '../network-filter-selector/style'

export interface Props {
  selected: boolean
  account: WalletAccountType
  onSelectAccount: (account?: WalletAccountType) => void
}

export const AccountFilterItem = (props: Props) => {
  const { account, selected, onSelectAccount } = props

  // Methods
  const onClickSelectAccount = React.useCallback(() => {
    onSelectAccount(account)
  }, [account, onSelectAccount])

  // Memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  return (
    <NetworkItemWrapper>
      <NetworkItemButton onClick={onClickSelectAccount}>
        <LeftSide>
          {account.address !== AllAccountsOption.address && (
            <AccountCircle orb={orb} />
          )}
          <NetworkName>
            {account.name}
          </NetworkName>
        </LeftSide>
        {selected &&
          <BigCheckMark />}
      </NetworkItemButton>
    </NetworkItemWrapper>
  )
}

export default AccountFilterItem
