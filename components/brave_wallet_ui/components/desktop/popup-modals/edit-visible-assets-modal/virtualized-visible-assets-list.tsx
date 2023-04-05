// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { VariableSizeList as List } from 'react-window'

// types
import { BraveWallet } from '../../../../constants/types'

// components
import AssetWatchlistItem from '../../asset-watchlist-item'

// styles
import {
  tokenListHeight
} from './style'
import { assetWatchListItemHeight } from '../../asset-watchlist-item/style'

interface VirtualizedTokensListProps {
  tokenList: BraveWallet.BlockchainToken[]
  isCustomToken: (token: BraveWallet.BlockchainToken) => boolean
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isAssetSelected: (token: BraveWallet.BlockchainToken) => boolean
  onCheckWatchlistItem: (key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => void
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'tokenList'> {
  index: number
  data: BraveWallet.BlockchainToken
  style: React.CSSProperties
  setSize: (index: number, size: number) => void
}

const getListItemKey = (index: number, tokenList: BraveWallet.BlockchainToken[]) => {
  const token = tokenList[index]
  return `${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`
}

const ListItem = (props: ListItemProps) => {
  const {
    data,
    style,
    index,
    isCustomToken,
    isAssetSelected,
    onCheckWatchlistItem,
    onRemoveAsset,
    setSize
  } = props

  const handleSetSize = React.useCallback((ref: HTMLDivElement | null) => {
    if (ref) {
      setSize(index, ref.getBoundingClientRect().height)
    }
  }, [index, setSize])

  return (
    <div style={style}>
      <AssetWatchlistItem
        ref={handleSetSize}
        isCustom={isCustomToken(data)}
        token={data}
        onRemoveAsset={onRemoveAsset}
        isSelected={isAssetSelected(data)}
        onSelectAsset={onCheckWatchlistItem}
      />
    </div>
  )
}

export const VirtualizedVisibleAssetsList = (props: VirtualizedTokensListProps) => {
  const {
    tokenList,
    isCustomToken,
    isAssetSelected,
    onCheckWatchlistItem,
    onRemoveAsset
  } = props

  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(new Array(tokenList.length).fill(assetWatchListItemHeight))

  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (itemSizes.current[index] !== size && size > -1) {
      itemSizes.current[index] = size
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [])

  const getSize = React.useCallback((index) => {
    return itemSizes.current[index] || assetWatchListItemHeight
  }, [])

  return (
    <List
      ref={listRef}
      width={'100%'}
      height={tokenListHeight}
      itemData={tokenList}
      itemCount={tokenList.length}
      itemSize={getSize}
      overscanCount={10}
      itemKey={getListItemKey}
      children={({ data, index, style }) => (
        <ListItem
          data={data[index]}
          isCustomToken={isCustomToken}
          onRemoveAsset={onRemoveAsset}
          isAssetSelected={isAssetSelected}
          onCheckWatchlistItem={onCheckWatchlistItem}
          setSize={setSize}
          index={index}
          style={style}
        />
      )}
    />
  )
}
