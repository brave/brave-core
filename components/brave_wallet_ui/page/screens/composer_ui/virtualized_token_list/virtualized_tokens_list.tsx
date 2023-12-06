// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from '@brave/react-virtualized-auto-sizer'

// Types
import { BraveWallet } from '../../../../constants/types'

// Components
import {
  TokenListItem //
} from '../token_list_item/token_list_item'

// Styled Components
import { ListItemWrapper } from './virtualized_tokens_list.style'

interface VirtualizedTokensListProps {
  tokenList: BraveWallet.BlockchainToken[]
  selectedToken?: BraveWallet.BlockchainToken
  onSelectToken: (token: BraveWallet.BlockchainToken) => void
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'tokenList'> {
  index: number
  data: BraveWallet.BlockchainToken[]
  style: React.CSSProperties
}

const itemSize = 72

const getListItemKey = (
  index: number,
  tokenList: BraveWallet.BlockchainToken[]
) => {
  const token = tokenList[index]
  return `${token.contractAddress}-${token.symbol}-${token.chainId}`
}

const ListItem = (props: ListItemProps) => {
  const { index, data, onSelectToken, style, selectedToken } = props
  const token = data[index]

  const disabledText =
    selectedToken?.contractAddress === token.contractAddress &&
    selectedToken?.coin === token.coin
      ? 'braveWalletFromToken'
      : undefined

  return (
    <ListItemWrapper style={style}>
      <TokenListItem
        onClick={() => onSelectToken(token)}
        token={token}
        disabledText={disabledText}
      />
    </ListItemWrapper>
  )
}

export const VirtualizedTokenList = (props: VirtualizedTokensListProps) => {
  const { tokenList, onSelectToken, selectedToken } = props

  return (
    <AutoSizer
      style={{
        height: '100%',
        width: '100%'
      }}
    >
      {function ({ height, width }) {
        return (
          <List
            width={width}
            height={height}
            itemData={tokenList}
            itemSize={(index: number) => itemSize}
            estimatedItemSize={itemSize}
            itemCount={tokenList.length}
            overscanCount={2}
            itemKey={getListItemKey}
            children={(itemProps) => (
              <ListItem
                {...itemProps}
                selectedToken={selectedToken}
                onSelectToken={onSelectToken}
              />
            )}
          />
        )
      }}
    </AutoSizer>
  )
}
