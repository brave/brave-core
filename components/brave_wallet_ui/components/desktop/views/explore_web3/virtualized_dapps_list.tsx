// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from '@brave/react-virtualized-auto-sizer'
import { color } from '@brave/leo/tokens/css'

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
  style: React.CSSProperties
}

const VirtualListStyle: React.CSSProperties = {
  flex: 1,
  width: '100%'
}

const defaultItemSizePx = 64
const getItemSize = () => defaultItemSizePx

const getListItemKey = (index: number, dappsList: BraveWallet.Dapp[]) => {
  return dappsList[index].id
}

const DappListItem = React.forwardRef<
  HTMLDivElement,
  {
    dapp: BraveWallet.Dapp
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
          color={color.text.tertiary}
          maxLines={1}
        >
          {dapp.description}
        </TextWithOverflowEllipsis>
      </Column>
    </Row>
  )
})

const ListItem = (props: ListItemProps) => {
  const { data, style, onClickDapp } = props

  return (
    <div style={style}>
      <DappListItem
        dapp={data}
        onClick={onClickDapp}
      />
    </div>
  )
}

export const VirtualizedDappsList = (props: VirtualizedTokensListProps) => {
  const { dappsList: tokenList, onClickDapp } = props

  // render
  return (
    <div style={VirtualListStyle}>
      <AutoSizer style={VirtualListStyle}>
        {function ({ width, height }) {
          return (
            <List
              width={width}
              height={height}
              itemData={tokenList}
              itemCount={tokenList.length}
              itemSize={getItemSize}
              overscanCount={10}
              itemKey={getListItemKey}
              children={({ data, index, style }) => (
                <ListItem
                  data={data[index]}
                  onClickDapp={onClickDapp}
                  index={index}
                  style={style}
                />
              )}
            />
          )
        }}
      </AutoSizer>
    </div>
  )
}
