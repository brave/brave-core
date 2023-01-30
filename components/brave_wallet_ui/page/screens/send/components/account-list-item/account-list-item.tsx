// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Styled Components
import { Button, AccountCircle } from './account-list-item.style'
import { Text, Column } from '../../shared.styles'

interface Props {
  onClick: (address: string) => void
  address: string
  name: string
}

export const AccountListItem = (props: Props) => {
  const { onClick, address, name } = props

  // Selectors
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)

  // Memos
  const isAccountDisabled = React.useMemo(() => {
    return selectedAccount?.address.toLowerCase() === address.toLowerCase()
  }, [selectedAccount, address])

  const orb = React.useMemo(() => {
    return create({ seed: address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [address])

  return (
    <Button disabled={isAccountDisabled} onClick={() => onClick(address)}>
      <AccountCircle orb={orb} />
      <Column horizontalAlign='flex-start' verticalAlign='center'>
        <Text textColor='text03' textSize='12px' isBold={false}>{name}</Text>
        <Text textColor='text01' textSize='12px' isBold={false}>{address}</Text>
      </Column>
    </Button>

  )
}

export default AccountListItem
