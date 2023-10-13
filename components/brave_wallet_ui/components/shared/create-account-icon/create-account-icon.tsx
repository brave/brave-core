// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Utils
import {
  getRewardsProviderIcon
} from '../../../utils/rewards_utils'

// Types
import { BraveWallet } from '../../../constants/types'
import {
  ExternalWalletProvider
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// styles
import {
  AccountBox,
  AccountIcon,
  ExternalAccountBox,
  ExternalAccountIcon
} from './create-account-icon.style'
import { useAccountOrb } from '../../../common/hooks/use-orb'

interface Props {
  account?: BraveWallet.AccountInfo
  externalProvider?: ExternalWalletProvider | null
  size?: 'big' | 'medium' | 'small' | 'tiny'
  marginRight?: number
  round?: boolean
}

export const CreateAccountIcon = (props: Props) => {
  const {
    account,
    size,
    marginRight,
    round,
    externalProvider
  } = props

  // Memos
  const orb = useAccountOrb(account)

  if (externalProvider) {
    return (
      <ExternalAccountBox
        size={size}
        marginRight={marginRight}
        round={round}
        provider={externalProvider}
      >
        <ExternalAccountIcon
          src={getRewardsProviderIcon(externalProvider)}
          size={size}
        />
      </ExternalAccountBox>
    )
  }

  return (
    <AccountBox
      orb={orb}
      size={size}
      marginRight={marginRight}
      round={round}
    >
      {account?.accountId.kind === BraveWallet.AccountKind.kHardware &&
        <AccountIcon
          name='flashdrive'
          size={size}
        />
      }
    </AccountBox>
  )
}
