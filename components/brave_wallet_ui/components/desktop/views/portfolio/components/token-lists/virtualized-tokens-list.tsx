// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import AutoSizer from '@brave/react-virtualized-auto-sizer'
import {
  // FixedSizeList,
  VariableSizeList as List
} from 'react-window'
import { BraveWallet, UserAssetInfoType } from '../../../../../../constants/types'

export const DynamicListContext = React.createContext<
  Partial<{ setSize: (index: number, size: number) => void }>
>({})

type ViewMode = 'list' | 'grid'

type VirtualizedTokensListProps<T extends UserAssetInfoType | BraveWallet.BlockchainToken = UserAssetInfoType> = {
  userAssetList: T[]
  renderToken: (item: T, viewMode: ViewMode) => JSX.Element
}

type ListItemProps<T extends UserAssetInfoType | BraveWallet.BlockchainToken = UserAssetInfoType> = {
  index: number
  // width: number
  data: T
  style: React.CSSProperties
  renderToken: (item: T, viewMode: ViewMode) => JSX.Element
}

// Component for each item. Measures height to let virtual
// list know.
const ListItem = ({
  data,
  index,
  renderToken,
  style
}: ListItemProps) => {
  const { setSize } = React.useContext(DynamicListContext)
  const rowRoot = React.useRef<null | HTMLDivElement>(null)

  React.useEffect(() => {
    if (rowRoot.current) {
      const marginBottom = parseFloat(getComputedStyle(rowRoot.current).marginBottom || '0')

      setSize && setSize(index, rowRoot.current.getBoundingClientRect().height + marginBottom)
    }
  }, [
    rowRoot,
    index,
    setSize
  ])

  const token = data[index]

  console.log({
    data,
    token
  })

  if (!token) {
    console.warn('Token was null at index',
      index,
      data
    )
    return null
  }

  return (
    <div ref={rowRoot} key={`token.asset.tokenId-${index}`}>
      <div style={style}>
        {renderToken(token, 'list')}
      </div>
    </div>
  )
}

export const VirtualizedTokensList = ({
  renderToken,
  userAssetList
}: VirtualizedTokensListProps) => {
  // refs
  const listRef = React.useRef<List | null>(null)
  const sizeMap = React.useRef<{ [key: string]: number }>({})

  // methods
  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value changed
    if (sizeMap.current[index] !== size) {
      sizeMap.current = { ...sizeMap.current, [index]: size }
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [sizeMap.current, listRef.current])

  const getSize = React.useCallback((index) => {
    const size = sizeMap.current[index] || 42
    alert(`size: ${size}`)
    return size
  }, [sizeMap.current])

  // render
  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <AutoSizer
        defaultHeight={150}
        defaultWidth={450}
        children={({ height, width }) => (
          <List
            ref={listRef}
            // width={width}
            height={height}
            width={'100%'}
            // height={100}
            itemSize={getSize}
            itemData={userAssetList}
            itemCount={userAssetList.length}
            // itemSize={42}
            overscanCount={1}
            style={{
              overscrollBehavior: 'contain'
            }}
            children={(itemProps) => (
              <ListItem
                {...itemProps}
                // width={width}
                // width={'100%'}
                renderToken={renderToken}
              />
            )}
          />
        )}
      />
    </DynamicListContext.Provider>
  )
}
