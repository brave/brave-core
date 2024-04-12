// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

// Styles
import { listItemInitialHeight, AutoSizerStyle } from './token-list.style'

type ViewMode = 'list' | 'grid'

type RenderTokenProps<T> = {
  item: T
  viewMode: ViewMode
  index: number
  account?: BraveWallet.AccountInfo
  ref?: React.Ref<HTMLDivElement> | undefined
}

const getItemKey = (i: number, data: BraveWallet.BlockchainToken[]) =>
  getAssetIdKey(data[i])

export type RenderTokenFunc<T> = (
  props: RenderTokenProps<T>
) => JSX.Element | undefined | null

type VirtualizedTokensListProps<T extends any[]> = {
  userAssetList: T
  selectedAssetId: string
  renderToken: RenderTokenFunc<T[number]>
}

type ListItemProps<T> = {
  index: number
  data: T
  style: React.CSSProperties
  setSize: (index: number, size: number) => void
  renderToken: RenderTokenFunc<T>
}

const ListItem = React.memo(function <T>({
  data,
  index,
  renderToken,
  setSize,
  style
}: ListItemProps<T>) {
  if (!data) {
    console.warn('Token was null at index', index, data)
  }

  const handleSetSize = React.useCallback(
    (ref: HTMLDivElement | null) => {
      if (ref) {
        setSize(index, ref.getBoundingClientRect().height)
      }
    },
    [index, setSize]
  )

  return (
    <div style={style}>
      {data &&
        renderToken({
          index,
          item: data,
          viewMode: 'list',
          ref: handleSetSize
        })}
    </div>
  )
})

export const VirtualizedTokensList = React.memo(function <T extends any[]>({
  renderToken,
  userAssetList,
  selectedAssetId
}: VirtualizedTokensListProps<T>) {
  // Refs
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(userAssetList.length).fill(listItemInitialHeight)
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
      // Get selectedAssetId index
      if (listRef.current && selectedAssetId) {
        const itemIndex = userAssetList.findIndex(
          (asset) => getAssetIdKey(asset) === selectedAssetId
        )
        // Scroll selected asset into view
        if (itemIndex > -1) {
          listRef.current.scrollToItem(itemIndex)
        }
      }
    },
    [listRef, selectedAssetId, userAssetList]
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

  // render
  return (
    <AutoSizer style={AutoSizerStyle}>
      {function ({ height, width }: { height: number; width: number }) {
        return (
          <List
            ref={onListMount}
            width={width}
            height={height}
            itemSize={getSize}
            itemData={userAssetList}
            itemCount={userAssetList.length}
            overscanCount={10}
            itemKey={getItemKey}
            children={(itemProps) => (
              <ListItem
                {...itemProps}
                data={itemProps.data[itemProps.index]}
                renderToken={renderToken}
                setSize={setSize}
              />
            )}
          />
        )
      }}
    </AutoSizer>
  )
})
