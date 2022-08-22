// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList } from 'react-window'

// types
import { BraveWallet, UserAssetInfoType } from '../../../../../../constants/types'
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

// styles
import { StyledDiv } from './virtualized-tokens-list.styles'

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
    rowRoot.current,
    index,
    setSize
  ])

  const token = data[index] as UserAssetInfoType

  if (!token) {
    console.warn('Token was null at index',
      index,
      data
    )
  }

  const isTopStyleBroken = typeof style.top !== 'string' && isNaN(Number(style.top))
  const isHeightBroken = typeof style.height !== 'string' && isNaN(Number(style.height))

  // prevent CSS NaN error
  let _style = style
  if (isTopStyleBroken || isHeightBroken) {
    if (isTopStyleBroken) {
      _style.top = '0px'
    }
    if (isHeightBroken) {
      _style.height = 'auto'
    }
  }

  const key = `${token?.asset.contractAddress
    }-${token?.asset.chainId
    }-${token?.asset.tokenId}`

  return (
    <div
      ref={rowRoot}
      key={key}
      style={_style}
    >
      {token && renderToken({ index, item: token, viewMode: 'list' })}
    </div>
  )
}

export const VirtualizedTokensList = ({
  renderToken,
  userAssetList,
  estimatedItemSize
}: VirtualizedTokensListProps) => {
  // refs
  const listRef = React.useRef<VariableSizeList | null>(null)
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
  const minimumItems = (Math.min(5, userAssetList.length || 1)) // min: 1, max: 5
  const listHeight = estimatedItemSize * minimumItems

  // render
  return (
    <DynamicListContext.Provider
      value={{ setSize }}
    >
      <VariableSizeList
        ref={listRef}
        width={'100%'}
        height={listHeight}
        itemSize={getSize}
        itemData={userAssetList}
        itemCount={userAssetList.length}
        estimatedItemSize={estimatedItemSize}
        overscanCount={20}
        itemKey={(i, data: UserAssetInfoType[]) => getAssetIdKey(data[i].asset)}
        innerElementType={StyledDiv} // Needed a custom div to suppress NaN CSS error
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
