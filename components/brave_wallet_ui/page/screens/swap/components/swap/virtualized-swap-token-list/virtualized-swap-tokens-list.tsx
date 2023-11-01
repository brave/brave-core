// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from '@brave/react-virtualized-auto-sizer'

import {
  useGetSelectedChainQuery //
} from '../../../../../../common/slices/api.slice'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Components
import {
  TokenListButton //
} from '../../buttons/token-list-button/token-list-button'
import Amount from '../../../../../../utils/amount'

interface VirtualizedTokensListProps {
  tokenList: BraveWallet.BlockchainToken[]
  onSelectToken: (token: BraveWallet.BlockchainToken) => void
  getCachedAssetBalance: (token: BraveWallet.BlockchainToken) => Amount
  disabledToken:
    | Pick<BraveWallet.BlockchainToken, 'contractAddress'>
    | undefined
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
  const {
    index,
    data,
    getCachedAssetBalance,
    disabledToken,
    onSelectToken,
    style
  } = props
  const token = data[index]

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  return (
    <div style={style}>
      <TokenListButton
        network={selectedNetwork}
        onClick={onSelectToken}
        balance={getCachedAssetBalance(token)}
        token={token}
        disabled={
          disabledToken
            ? disabledToken.contractAddress === token.contractAddress
            : false
        }
      />
    </div>
  )
}

export const VirtualizedTokenList = (props: VirtualizedTokensListProps) => {
  const { tokenList, disabledToken, getCachedAssetBalance, onSelectToken } =
    props

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
                getCachedAssetBalance={getCachedAssetBalance}
                disabledToken={disabledToken}
                onSelectToken={onSelectToken}
              />
            )}
          />
        )
      }}
    </AutoSizer>
  )
}
