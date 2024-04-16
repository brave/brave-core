// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'

// types
import { BraveWallet } from '../../../../constants/types'

// styles
import { Column, Row, Text } from '../../../shared/style'
import {
  TextWithOverflowEllipsis //
} from '../../../../page/screens/send/shared.styles'

interface VirtualizedTokensListProps {
  dappsList: BraveWallet.Dapp[]
  onClickDapp: (dappId: number) => void
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'dappsList'> {
  index: number
  data: BraveWallet.Dapp
  setSize: (index: number, size: number) => void
  style: React.CSSProperties
}

const VirtualListStyle: React.CSSProperties = {
  flex: 1,
  width: '100%'
}

const defaultItemSizePx = 64

const getListItemKey = (index: number, dappsList: BraveWallet.Dapp[]) => {
  return dappsList[index].id
}

const DappListItem = React.forwardRef<
  HTMLDivElement,
  {
    dapp: BraveWallet.Dapp
    ref: React.Ref<HTMLDivElement>
    onClick?: (dappId: number) => void
  }
>(({ dapp, onClick }, ref) => {
  return (
    <Row
      ref={ref}
      gap={'8px'}
      padding={'12px 8px'}
      width='100%'
      justifyContent='flex-start'
      onClick={
        onClick
          ? function () {
              onClick(dapp.id)
            }
          : undefined
      }
    >
      <Column>
        <img
          src={`chrome://image?${dapp.logo}`}
          height={40}
          width={40}
        />
      </Column>
      <Column
        alignItems='flex-start'
        justifyContent='flex-start'
      >
        <Text
          textAlign='left'
          isBold
          textSize='14px'
        >
          {dapp.name}
        </Text>

        <TextWithOverflowEllipsis
          textSize='12px'
          textAlign='left'
          textColor='tertiary'
          maxLines={1}
        >
          {dapp.description}
        </TextWithOverflowEllipsis>
      </Column>
    </Row>
  )
})

const ListItem = (props: ListItemProps) => {
  const { index, data, style, setSize, onClickDapp } = props

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
      <DappListItem
        dapp={data}
        onClick={onClickDapp}
        ref={handleSetSize}
      />
    </div>
  )
}

export const VirtualizedDappsList = (props: VirtualizedTokensListProps) => {
  const { dappsList: tokenList, onClickDapp } = props
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(tokenList.length).fill(defaultItemSizePx)
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
      return itemSizes.current[index] || defaultItemSizePx
    },
    [itemSizes]
  )

  // render
  return (
    <div style={VirtualListStyle}>
      <AutoSizer style={VirtualListStyle}>
        {function ({ width, height }: { width: number; height: number }) {
          return (
            <List
              ref={onListMount}
              width={width}
              height={height}
              itemData={tokenList}
              itemCount={tokenList.length}
              itemSize={getSize}
              overscanCount={10}
              itemKey={getListItemKey}
              children={({ data, index, style }) => (
                <ListItem
                  data={data[index]}
                  onClickDapp={onClickDapp}
                  index={index}
                  style={style}
                  setSize={setSize}
                />
              )}
            />
          )
        }}
      </AutoSizer>
    </div>
  )
}
