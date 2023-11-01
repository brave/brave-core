// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Styled Components
import { AccountCircle } from './account-filter-selector.style'

import {
  NetworkItemButton,
  NetworkName,
  NetworkItemWrapper,
  LeftSide,
  BigCheckMark
} from '../network-filter-selector/style'

// Hooks
import { useAccountOrb } from '../../../common/hooks/use-orb'

export interface Props {
  selected: boolean
  showCircle: boolean
  account: BraveWallet.AccountInfo
  onSelectAccount: (account?: BraveWallet.AccountInfo) => void
}

export const AccountFilterItem = (props: Props) => {
  const { account, selected, showCircle, onSelectAccount } = props

  // Methods
  const onClickSelectAccount = React.useCallback(() => {
    onSelectAccount(account)
  }, [account, onSelectAccount])

  // Memos
  const orb = useAccountOrb(account)

  return (
    <NetworkItemWrapper>
      <NetworkItemButton onClick={onClickSelectAccount}>
        <LeftSide>
          {showCircle && <AccountCircle orb={orb} />}
          <NetworkName>{account.name}</NetworkName>
        </LeftSide>
        {selected && <BigCheckMark />}
      </NetworkItemButton>
    </NetworkItemWrapper>
  )
}

export default AccountFilterItem
