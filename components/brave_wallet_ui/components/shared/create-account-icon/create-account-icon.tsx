// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// styles
import {
  AccountBox,
  AccountIcon
} from './create-account-icon.style'
import { useAccountOrb } from '../../../common/hooks/use-orb'

interface Props {
  account: BraveWallet.AccountInfo
  size?: 'big' | 'medium' | 'small' | 'tiny'
  marginRight?: number
}

export const CreateAccountIcon = (props: Props) => {
  const {
    account,
    size,
    marginRight,
  } = props

  // Memos
  const orb = useAccountOrb(account)

  return (
    <AccountBox
      orb={orb}
      size={size}
      marginRight={marginRight}
    >
      {account.accountId.kind === BraveWallet.AccountKind.kHardware &&
        <AccountIcon
          name='flashdrive'
          size={size}
        />
      }
    </AccountBox>
  )
}
