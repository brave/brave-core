// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'

// Types
import { BraveWallet } from '../../../../constants/types'

// Components
import {
  TokenListItem //
} from '../token_list_item/token_list_item'

// Styled Components
import {
  AutoSizerStyle,
  ListItemWrapper,
  listItemInitialHeight
} from './virtualized_tokens_list.style'

interface VirtualizedTokensListProps {
  tokenList: BraveWallet.BlockchainToken[]
  selectedToken?: BraveWallet.BlockchainToken
  onSelectToken: (token: BraveWallet.BlockchainToken) => void
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'tokenList'> {
  index: number
  data: BraveWallet.BlockchainToken[]
  style: React.CSSProperties
  setSize: (index: number, size: number) => void
}

const getListItemKey = (
  index: number,
  tokenList: BraveWallet.BlockchainToken[]
) => {
  const token = tokenList[index]
  return `${token.contractAddress}-${token.symbol}-${token.chainId}`
}

const ListItem = (props: ListItemProps) => {
  const { index, data, onSelectToken, style, selectedToken, setSize } = props
  const token = data[index]

  const disabledText =
    selectedToken?.contractAddress === token.contractAddress &&
    selectedToken?.coin === token.coin
      ? 'braveWalletFromToken'
      : undefined

  const handleSetSize = React.useCallback(
    (ref: HTMLDivElement | null) => {
      if (ref) {
        setSize(index, ref.getBoundingClientRect().height)
      }
    },
    [index, setSize]
  )

  return (
    <ListItemWrapper style={style}>
      <TokenListItem
        onClick={() => onSelectToken(token)}
        token={token}
        disabledText={disabledText}
        ref={handleSetSize}
      />
    </ListItemWrapper>
  )
}

export const VirtualizedTokenList = (props: VirtualizedTokensListProps) => {
  const { tokenList, onSelectToken, selectedToken } = props

  // Refs
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(tokenList.length).fill(listItemInitialHeight)
  )

  // Methods
  const onListMount = React.useCallback(
    (ref: List | null) => {
      if (ref) {
        // Set ref on mount
        listRef.current = ref
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    },
    [listRef]
  )

  const setSize = React.useCallback(
    (index: number, size: number) => {
      // Performance: Only update the sizeMap and reset cache if an actual value
      // changed
      if (itemSizes.current[index] !== size && size > -1) {
        itemSizes.current[index] = size
        if (listRef.current) {
          // Clear cached data and rerender
          listRef.current.resetAfterIndex(0)
        }
      }
    },
    [itemSizes, listRef]
  )

  const getSize = React.useCallback(
    (index: number) => {
      return itemSizes.current[index] || listItemInitialHeight
    },
    [itemSizes]
  )

  return (
    <AutoSizer style={AutoSizerStyle}>
      {function ({ height, width }: { height: number; width: number }) {
        return (
          <List
            ref={onListMount}
            width={width}
            height={height}
            itemData={tokenList}
            itemCount={tokenList.length}
            itemSize={getSize}
            itemKey={getListItemKey}
            overscanCount={10}
            children={(itemProps) => (
              <ListItem
                {...itemProps}
                selectedToken={selectedToken}
                onSelectToken={onSelectToken}
                setSize={setSize}
              />
            )}
          />
        )
      }}
    </AutoSizer>
  )
}
