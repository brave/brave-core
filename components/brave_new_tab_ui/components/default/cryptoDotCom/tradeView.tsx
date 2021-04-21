// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { AssetViews } from './types'
import {
  Box,
  FlexItem,
  ButtonGroup,
  InputField,
  List,
  ListItem,
  PlainButton,
  Text
} from './style'

import {
  formattedNum,
  getPercentColor,
  renderIconAsset,
  transformLosersGainers
} from './utils'

import { SearchIcon } from '../../default/exchangeWidget/shared-assets'
import { getLocale } from '../../../../common/locale'

interface Props {
  tickerPrices: Record<string, chrome.cryptoDotCom.TickerPrice>
  losersGainers: Record<string, chrome.cryptoDotCom.AssetRanking[]>
  tradingPairs: chrome.cryptoDotCom.SupportedPair[]
  handleAssetClick: (base: string, quote?: string, view?: AssetViews) => void
}

export default function TradeView ({
  tickerPrices,
  losersGainers,
  tradingPairs,
  handleAssetClick
}: Props) {
  const assetRankings = transformLosersGainers(losersGainers)

  enum FilterValues {
    ALL = 'ALL',
    BTC = 'BTC',
    CRO = 'CRO',
    USDT = 'USDT'
  }

  const [filter, setFilter] = React.useState(FilterValues.ALL)
  const [searchInput, setSearchInput] = React.useState('')

  const handleSearchChange = ({ target }: any) => {
    const { value } = target
    setSearchInput(value)
  }

  const searchFilter = (pair: Record<string, string>) => {
    if (searchInput) {
      const search = new RegExp(searchInput, 'i')
      return search.test(pair.base)
    } else {
      return pair
    }
  }

  const renderListItem = (pair: Record<string, string>) => {
    const { price = null } = tickerPrices[pair.pair] || {}
    const { percentChange = null } = assetRankings[pair.base] || {}

    return (
      <ListItem key={pair.pair} isFlex={true} $height={48} onClick={handleAssetClick.bind(this, pair.base, pair.quote, AssetViews.TRADE)}>
        <FlexItem $pl={5} $pr={5}>
          {renderIconAsset(pair.base.toLowerCase())}
        </FlexItem>
        <FlexItem>
          <Text>{pair.base}/{pair.quote}</Text>
        </FlexItem>
        <FlexItem textAlign='right' flex={1}>
          {(price !== null) && <Text>
            {filter === FilterValues.USDT ? formattedNum(price) : price}
          </Text>}
          {(percentChange !== null) && <Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
        </FlexItem>
      </ListItem>
    )
  }

  const renderAllView = () => {
    return tradingPairs.filter(searchFilter)
                       .map(renderListItem)
  }

  const renderFilterView = () => {
    return tradingPairs
      .filter((pair: Record<string, string>) => pair.quote === filter)
      .filter(searchFilter)
      .map(renderListItem)
  }

  return (
    <>
      <ButtonGroup>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.ALL)}
          inButtonGroup={true}
          isActive={filter === FilterValues.ALL}
        >
          ALL
        </PlainButton>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.BTC)}
          inButtonGroup={true}
          isActive={filter === FilterValues.BTC}
        >
          BTC
        </PlainButton>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.CRO)}
          inButtonGroup={true}
          isActive={filter === FilterValues.CRO}
        >
          CRO
        </PlainButton>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.USDT)}
          inButtonGroup={true}
          isActive={filter === FilterValues.USDT}
        >
          USDT
        </PlainButton>
      </ButtonGroup>
      <Box isFlex={true} $height={30} hasBottomBorder={false}>
        <img width={15} src={SearchIcon} />
        <InputField value={searchInput} onChange={handleSearchChange} placeholder={getLocale('cryptoDotComWidgetSearch')} />
      </Box>
      <List>
        {filter === FilterValues.ALL ? renderAllView() : renderFilterView()}
      </List>
    </>
  )
}
