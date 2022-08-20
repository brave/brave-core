// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'

import { BraveWallet, UserAssetInfoType } from '../../../../../../constants/types'

type ViewMode = 'list' | 'grid'

type RenderTokenProps = {
  item: UserAssetInfoType
  viewMode: ViewMode
  index: number
}

export type RenderTokenFunc = (props: RenderTokenProps) => JSX.Element

type VirtualizedTokensListProps = {
  userAssetList: UserAssetInfoType[]
  renderToken: RenderTokenFunc
  estimatedItemSize: number
}

type ListItemProps<T extends UserAssetInfoType | BraveWallet.BlockchainToken = UserAssetInfoType> = {
  index: number
  data: T
  style: React.CSSProperties
  renderToken: RenderTokenFunc
}

export const DynamicListContext = React.createContext<
  Partial<{ setSize: (index: number, size: number) => void }>
>({})

// Component for each item.
// Measures height to let virtual list know.
const ListItem = ({
  data,
  index,
  renderToken,
  style
}: ListItemProps) => {
  const { setSize } = React.useContext(DynamicListContext)
  const rowRoot = React.useRef<null | HTMLDivElement>(null)

  React.useEffect(() => {
    if (rowRoot.current && setSize) {
      const height = rowRoot.current.getBoundingClientRect().height
      setSize(index, height)
    }
  }, [
    rowRoot,
    index,
    setSize
  ])

  const token = data[index] as UserAssetInfoType

  if (!token) {
    console.warn('Token was null at index',
      index,
      data
    )
    return null
  }

  return (
    <div ref={rowRoot} key={`${token.asset.contractAddress}-${token.asset.chainId}-${token.asset.tokenId}`} style={style}>
      {renderToken({ index, item: token, viewMode: 'list' })}
    </div>
  )
}

export const VirtualizedTokensList = ({
  renderToken,
  userAssetList,
  estimatedItemSize
}: VirtualizedTokensListProps) => {
  // refs
  const listRef = React.useRef<List | null>(null)
  const sizeMap = React.useRef<number[]>(
    Array(userAssetList.length).fill(estimatedItemSize) // assume estimated size at first
  )

  // methods
  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (!isNaN(size) && sizeMap.current[index] !== size) {
      sizeMap.current[index] = size
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [sizeMap, listRef])

  const getSize = React.useCallback((index: number) => sizeMap.current[index], [])

  // computed
  const minimumItems = (Math.min(10, userAssetList.length || 1)) // min: 1, max: 10
  const listHeight = estimatedItemSize * minimumItems

  // render
  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <List
        ref={listRef}
        width='100%'
        height={listHeight}
        itemSize={getSize}
        itemData={userAssetList}
        itemCount={userAssetList.length}
        estimatedItemSize={estimatedItemSize}
        overscanCount={20}
        style={{
          overscrollBehavior: 'contain'
        }}
        children={(itemProps) => (
          <ListItem
            {...itemProps}
            renderToken={renderToken}
          />
        )}
      />
    </DynamicListContext.Provider>
  )
}
