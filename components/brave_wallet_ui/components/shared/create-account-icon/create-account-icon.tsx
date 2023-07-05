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
import { useAddressOrb } from '../../../common/hooks/use-orb'

interface Props {
  address: string
  size?: 'big' | 'medium' | 'small' | 'tiny'
  marginRight?: number
  accountKind?: BraveWallet.AccountKind
}

export const CreateAccountIcon = (props: Props) => {
  const {
    address,
    size,
    marginRight,
    accountKind
  } = props

  // Memos
  const orb = useAddressOrb(address)

  return (
    <AccountBox
      orb={orb}
      size={size}
      marginRight={marginRight}
    >
      {accountKind === BraveWallet.AccountKind.kHardware &&
        <AccountIcon
          name='flashdrive'
          size={size}
        />
      }
    </AccountBox>
  )
}
