// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { AssetViews } from './types'
import {
  formattedNum,
  getPercentColor,
} from './utils'

import {
  FlexItem,
  ButtonGroup,
  List,
  ListItem,
  PlainButton,
  Text
} from './style'

import IconAsset from '../../../widgets/shared/iconAsset'
import { currencyNames } from '../../../widgets/shared/data'
import { getLocale } from '../../../../common/locale'

interface Props {
  losersGainers: Record<string, chrome.cryptoDotCom.AssetRanking[]>
  handleAssetClick: (base: string, quote?: string, view?: AssetViews) => void
}

export default function TopMoversView ({
  losersGainers,
  handleAssetClick
}: Props) {
  enum FilterValues {
    LOSERS = 'losers',
    GAINERS = 'gainers'
  }

  const [filter, setFilter] = React.useState(FilterValues.GAINERS)

  const sortTopMovers = (a: chrome.cryptoDotCom.AssetRanking, b: chrome.cryptoDotCom.AssetRanking) => {
    if (filter === FilterValues.GAINERS) {
      return Number(b.percentChange) - Number(a.percentChange)
    } else {
      return Number(a.percentChange) - Number(b.percentChange)
    }
  }

  const renderListItem = (asset: chrome.cryptoDotCom.AssetRanking) => {
    const currency = asset.pair.split('_')[0]
    const { percentChange, lastPrice: price } = asset
    return (
      <ListItem key={currency} isFlex={true} onClick={handleAssetClick.bind(this, currency, undefined, AssetViews.DETAILS)} $height={48}>
        <FlexItem $pl={5} $pr={5}>
          <IconAsset iconKey={currency.toLowerCase()} />
        </FlexItem>
        <FlexItem>
          <Text>{currency}</Text>
          <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
        </FlexItem>
        <FlexItem textAlign='right' flex={1}>
          {<Text>{formattedNum(Number(price))}</Text>}
          {<Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
        </FlexItem>
      </ListItem>
    )
  }

  const renderFilteredView = () => {
    return (
      losersGainers && losersGainers[filter] && losersGainers[filter]
        .sort(sortTopMovers)
        .map(renderListItem)
    )
  }

  return (
    <>
      <ButtonGroup>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.GAINERS)}
          inButtonGroup={true}
          isActive={filter === FilterValues.GAINERS}
        >
          {getLocale('cryptoDotComWidgetGainers')}
        </PlainButton>
        <PlainButton
          onClick={setFilter.bind(this, FilterValues.LOSERS)}
          inButtonGroup={true}
          isActive={filter === FilterValues.LOSERS}
        >
          {getLocale('cryptoDotComWidgetLosers')}
        </PlainButton>
      </ButtonGroup>
      <List>
        {renderFilteredView()}
      </List>
    </>
  )
}
