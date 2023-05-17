// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create } from 'ethereum-blockies'

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

interface Props {
  name: string
  address: string
  onClick: () => void
}

export const AccountListItemButton = (props: Props) => {
  const { onClick, address, name } = props

  // Memos
  const accountOrb: string = React.useMemo(() => {
    return create({
      seed: address.toLowerCase() || '',
      size: 8,
      scale: 16
    }).toDataURL()
  }, [address])

  return (
    <AccountButton onClick={onClick}>
      <AccountCircle orb={accountOrb} />{' '}
      <AccountText textSize='14px' isBold={true}>
        {name}
      </AccountText>
      <HorizontalSpacer size={4} />
      <Text textSize='14px' textColor='text03' isBold={false}>
        {reduceAddress(address)}
      </Text>
    </AccountButton>
  )
}
