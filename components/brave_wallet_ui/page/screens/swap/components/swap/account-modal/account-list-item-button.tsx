// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../../../../constants/types'

// Utils
import {
  reduceAddress
} from '../../../../../../utils/reduce-address'

// Styled Components
import {
  AccountButton,
  AccountCircle,
  AccountText
} from './account-modal.style'
import {
  Text,
  HorizontalSpacer,
} from '../../shared-swap.styles'

// Hooks
import { useAccountOrb } from '../../../../../../common/hooks/use-orb'

interface Props {
  account: BraveWallet.AccountInfo
  onClick: () => void
}

export const AccountListItemButton = (props: Props) => {
  const { onClick, account } = props

  const accountOrb = useAccountOrb(account)

  return (
    <AccountButton onClick={onClick}>
      <AccountCircle orb={accountOrb} />{' '}
      <AccountText textSize='14px' isBold={true}>
        {account.name}
      </AccountText>
      <HorizontalSpacer size={4} />
      <Text textSize='14px' textColor='text03' isBold={false}>
        {reduceAddress(account.address)}
      </Text>
    </AccountButton>
  )
}
