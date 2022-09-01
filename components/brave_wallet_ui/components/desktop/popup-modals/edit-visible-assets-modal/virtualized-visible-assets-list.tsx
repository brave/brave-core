// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { FixedSizeList as List } from 'react-window'

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
  networkList: BraveWallet.NetworkInfo[]
  isCustomToken: (token: BraveWallet.BlockchainToken) => boolean
  onRemoveAsset: (token: BraveWallet.BlockchainToken) => void
  isAssetSelected: (token: BraveWallet.BlockchainToken) => boolean
  onCheckWatchlistItem: (key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => void
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'tokenList'>{
  index: number
  data: BraveWallet.BlockchainToken[]
  style: React.CSSProperties
}

const getListItemKey = (index: number, tokenList: BraveWallet.BlockchainToken) => {
  const token = tokenList[index]
  return `${token.contractAddress}-${token.symbol}-${token.chainId}-${token.tokenId}`
}

const ListItem = (props: ListItemProps) => {
  const {
    index,
    data,
    style,
    networkList,
    isCustomToken,
    isAssetSelected,
    onCheckWatchlistItem,
    onRemoveAsset
  } = props
  const token = data[index]

  return (
    <div style={style}>
      <AssetWatchlistItem
        isCustom={isCustomToken(token)}
        token={token}
        networkList={networkList}
        onRemoveAsset={onRemoveAsset}
        isSelected={isAssetSelected(token)}
        onSelectAsset={onCheckWatchlistItem}
      />
    </div>
  )
}

export const VirtualizedVisibleAssetsList = (props: VirtualizedTokensListProps) => {
  const {
    tokenList,
    networkList,
    isCustomToken,
    isAssetSelected,
    onCheckWatchlistItem,
    onRemoveAsset
  } = props

  return (
    <List
      width={'100%'}
      height={tokenListHeight}
      itemData={tokenList}
      itemCount={tokenList.length}
      itemSize={assetWatchListItemHeight}
      overscanCount={10}
      itemKey={getListItemKey}
      children={(itemProps) => (
        <ListItem
          {...itemProps}
          isCustomToken={isCustomToken}
          networkList={networkList}
          onRemoveAsset={onRemoveAsset}
          isAssetSelected={isAssetSelected}
          onCheckWatchlistItem={onCheckWatchlistItem}
        />
      )}
    />
  )
}
