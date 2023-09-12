// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  VariableSizeList as List, //
  ListChildComponentProps
} from 'react-window'

// types
import {
  BraveWallet,
  UserAssetInfoType
} from '../../../../../../constants/types'
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

type ViewMode = 'list' | 'grid'

type RenderTokenProps = {
  item: UserAssetInfoType
  viewMode: ViewMode
  index: number
  account?: BraveWallet.AccountInfo
}

export type RenderTokenFunc = (props: RenderTokenProps) => JSX.Element | undefined | null

type VirtualizedTokensListProps = {
  userAssetList: UserAssetInfoType[]
  renderToken: RenderTokenFunc
  estimatedItemSize: number
  getItemSize: (index: number) => number
  maximumViewableTokens?: number
}

type ListItemProps<T extends UserAssetInfoType | BraveWallet.BlockchainToken = UserAssetInfoType> = {
  index: number
  data: T
  style: React.CSSProperties
  renderToken: RenderTokenFunc
}

const ListItem = ({
  data,
  index,
  renderToken,
  style
}: ListItemProps) => {
  if (!data) {
    console.warn('Token was null at index',
      index,
      data
    )
  }

  return (
    <div style={style}>
      {data && renderToken({ index, item: data, viewMode: 'list' })}
    </div>
  )
}

const LIST_STYLE = {
  overscrollBehavior: 'contain'
}

const getListItemKey = (i: number, data: UserAssetInfoType[]) => getAssetIdKey(data[i].asset)

export const VirtualizedTokensList = React.memo(({
  renderToken,
  userAssetList,
  estimatedItemSize,
  getItemSize,
  maximumViewableTokens
}: VirtualizedTokensListProps) => {
  // computed
  // last item shown as 50% visible to indicate that scrolling is possible here
  const maxTokens =
    maximumViewableTokens !== undefined
      ? maximumViewableTokens
      : 4.5
  const minimumItems =
    (Math.min(maxTokens, userAssetList.length || 1)) // min: 1, max: 4.5
  const listHeight = estimatedItemSize * minimumItems

  // methods
  const renderListChild = React.useCallback(
    (itemProps: ListChildComponentProps<UserAssetInfoType[]>) => (
      <ListItem
        data={itemProps.data[itemProps.index]}
        index={itemProps.index}
        renderToken={renderToken}
        style={itemProps.style}
      />
    ),
    [renderToken]
  )

  // render
  return (
    <List
      width={'100%'}
      height={listHeight}
      itemSize={getItemSize}
      itemData={userAssetList}
      itemCount={userAssetList.length}
      estimatedItemSize={estimatedItemSize}
      overscanCount={20}
      itemKey={getListItemKey}
      style={LIST_STYLE}
      children={renderListChild}
    />
  )
})
