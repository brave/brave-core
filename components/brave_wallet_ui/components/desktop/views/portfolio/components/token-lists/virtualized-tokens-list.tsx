// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  VariableSizeList as List, //
  ListChildComponentProps,
  ListItemKeySelector
} from 'react-window'

// types
import { BraveWallet } from '../../../../../../constants/types'

type ViewMode = 'list' | 'grid'

type RenderTokenProps<T> = {
  item: T
  viewMode: ViewMode
  index: number
  account?: BraveWallet.AccountInfo
}

export type RenderTokenFunc<T> = (
  props: RenderTokenProps<T>
) => JSX.Element | undefined | null

type VirtualizedTokensListProps<T extends any[]> = {
  userAssetList: T
  renderToken: RenderTokenFunc<T[number]>
  estimatedItemSize: number
  getItemSize: (index: number) => number
  getItemKey: ListItemKeySelector<T[number]>
  maximumViewableTokens?: number
}

type ListItemProps<T> = {
  index: number
  data: T
  style: React.CSSProperties
  renderToken: RenderTokenFunc<T>
}

const ListItem = React.memo(function <T>({
  data,
  index,
  renderToken,
  style
}: ListItemProps<T>) {
  if (!data) {
    console.warn('Token was null at index', index, data)
  }

  return (
    <div style={style}>
      {data && renderToken({ index, item: data, viewMode: 'list' })}
    </div>
  )
})

const LIST_STYLE = {
  overscrollBehavior: 'contain'
}

export const VirtualizedTokensList = React.memo(function <T extends any[]>({
  renderToken,
  userAssetList,
  estimatedItemSize,
  getItemSize,
  maximumViewableTokens,
  getItemKey
}: VirtualizedTokensListProps<T>) {
  // computed
  // last item shown as 50% visible to indicate that scrolling is possible here
  const maxTokens =
    maximumViewableTokens !== undefined ? maximumViewableTokens : 4.5
  /** min: 1, max: 4.5 */
  const minimumItems = Math.min(maxTokens, userAssetList.length || 1)
  const listHeight = estimatedItemSize * minimumItems

  // methods
  const renderListChild = React.useCallback(
    (itemProps: ListChildComponentProps<T>) => (
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
      itemKey={getItemKey}
      style={LIST_STYLE}
      children={renderListChild}
    />
  )
})
