// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import Amount from '../../../../../../utils/amount'

// Components
import {
  withPlaceholderIcon //
} from '../../../../../../components/shared/create-placeholder-icon'

// Styled Components
import { Button, NameAndSymbol } from './token-list-button.style'
import { Text, Row, AssetIcon } from '../../shared-swap.styles'

interface Props {
  onClick: (token: BraveWallet.BlockchainToken) => void
  token: BraveWallet.BlockchainToken
  network?: BraveWallet.NetworkInfo | null
  balance: Amount
  disabled: boolean
}

export const TokenListButton = (props: Props) => {
  const { onClick, token, balance, disabled, network } = props

  // Methods
  const onSelectToken = React.useCallback(() => {
    onClick(token)
  }, [token, onClick])

  // Memeos
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, {
      size: 'big',
      marginLeft: 0,
      marginRight: 12
    })
  }, [])

  return (
    <Button
      onClick={onSelectToken}
      disabled={disabled}
    >
      <Row>
        <AssetIconWithPlaceholder
          asset={token}
          network={network}
        />
        <NameAndSymbol horizontalAlign='flex-start'>
          <Text
            isBold={true}
            textColor='text01'
            textSize='14px'
            textAlign='left'
          >
            {token.name}
          </Text>
          <Text
            textColor='text03'
            textSize='14px'
            textAlign='left'
          >
            {token.symbol}
          </Text>
        </NameAndSymbol>
      </Row>
      <Text
        isBold={true}
        textColor='text01'
        textSize='14px'
      >
        {balance
          .divideByDecimals(token.decimals)
          .formatAsAsset(6, token.symbol)}
      </Text>
    </Button>
  )
}
