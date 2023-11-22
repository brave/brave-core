// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { reduceAddress } from '../../../../../../utils/reduce-address'

// Styled Components
import { Button } from './account-list-button.style'
import { Text, Row, HorizontalSpacer } from '../../shared-swap.styles'

interface Props {
  onClick: (account: BraveWallet.AccountInfo) => void
  account: BraveWallet.AccountInfo
}

export const AccountListButton = (props: Props) => {
  const { account, onClick } = props

  const onSelectAccount = React.useCallback(() => {
    onClick(account)
  }, [account, onClick])

  return (
    <Button onClick={onSelectAccount}>
      <Row>
        <Text
          isBold={true}
          textSize='14px'
          textColor='text01'
        >
          {account.name}
        </Text>
        <HorizontalSpacer size={15} />
        <Text
          isBold={false}
          textSize='12px'
          textColor='text03'
        >
          {reduceAddress(account.address)}
        </Text>
      </Row>
    </Button>
  )
}
