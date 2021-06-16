// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratLeftIcon } from 'brave-ui/components/icons'

import { AssetViews } from './types'
import {
  ActionButton,
  BackArrow,
  BasicBox,
  Box,
  FlexItem,
  SVG,
  Text,
  UpperCaseText
} from './style'

import {
  formattedNum,
  getPercentColor,
  transformLosersGainers
} from './utils'
import IconAsset from '../../../widgets/shared/iconAsset'
import { currencyNames } from '../../../widgets/shared/data'
import { getLocale } from '../../../../common/locale'

interface ChartConfig {
  chartData: chrome.cryptoDotCom.ChartDataPoint[]
  chartHeight: number
  chartWidth: number
}

function plotData ({ chartData, chartHeight, chartWidth }: ChartConfig) {
  const pointsPerDay = 4
  const daysInrange = 7
  const yHighs = chartData.map((point: chrome.cryptoDotCom.ChartDataPoint) => point.h)
  const yLows = chartData.map((point: chrome.cryptoDotCom.ChartDataPoint) => point.l)
  const dataPoints = chartData.map((point: chrome.cryptoDotCom.ChartDataPoint) => point.c)
  const chartAreaY = chartHeight - 2
  const max = Math.max(...yHighs)
  const min = Math.min(...yLows)
  const pixelsPerPoint = (max - min) / chartAreaY
  return dataPoints
    .map((v, i) => {
      const y = (v - min) / pixelsPerPoint
      const x = i * (chartWidth / (pointsPerDay * daysInrange))
      return `${x},${chartAreaY - y}`
    })
    .join('\n')
}

interface Props {
  currency: string
  losersGainers: Record<string, chrome.cryptoDotCom.AssetRanking[]>
  tickerPrice: chrome.cryptoDotCom.TickerPrice
  pairs: string[]
  chartData: chrome.cryptoDotCom.ChartDataPoint[]
  handleAssetClick: (base: string, quote?: string, view?: AssetViews) => void
  handleBackClick: () => void
}

export default function AssetDetailView ({
  currency,
  losersGainers,
  tickerPrice,
  pairs,
  chartData,
  handleAssetClick,
  handleBackClick
}: Props) {
  const { price = null, volume = null } = tickerPrice || {}
  const transformedlosersGainers = transformLosersGainers(losersGainers || {})
  const { percentChange = null } = transformedlosersGainers[currency] || {}

  const chartHeight = 100
  const chartWidth = 309
  return (
    <Box $p={0}>
      <FlexItem
        hasPadding={true}
        isFlex={true}
        isFullWidth={true}
        hasBorder={true}
      >
        <FlexItem>
          <BackArrow onClick={handleBackClick}>
            <CaratLeftIcon />
          </BackArrow>
        </FlexItem>
        <FlexItem $pr={5}>
          <IconAsset iconKey={currency.toLowerCase()} />
        </FlexItem>
        <FlexItem flex={1}>
          <Text>{currency}</Text>
          <Text small={true} textColor='light'>
            {currencyNames[currency]}
          </Text>
        </FlexItem>
        <FlexItem $pl={5}>
          <ActionButton onClick={handleAssetClick.bind(this, currency, undefined, AssetViews.DEPOSIT)} small={true} light={true}>
            <UpperCaseText>
            {getLocale('cryptoDotComWidgetDeposit')}
            </UpperCaseText>
          </ActionButton>
        </FlexItem>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
        hasBorder={true}
      >
        {(price !== null) && <Text
          inline={true}
          large={true}
          weight={500}
          $mr='0.5rem'
        >
          {formattedNum(price)} USDT
        </Text>}
        {(percentChange !== null) && <Text inline={true} textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
        <SVG viewBox={`0 0 ${chartWidth} ${chartHeight}`}>
          <polyline
            fill='none'
            stroke='#44B0FF'
            strokeWidth='3'
            points={plotData({
              chartData,
              chartHeight,
              chartWidth
            })}
          />
        </SVG>
      <Text small={true} textColor='xlight'>
        {getLocale('cryptoDotComWidgetGraph')}
      </Text>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
      >
        <BasicBox $mt='0.2em'>
          <Text small={true} textColor='light' $pb='0.2rem'>
            <UpperCaseText>
              {getLocale('cryptoDotComWidgetVolume')}
            </UpperCaseText>
          </Text>
          {volume && <Text weight={500}>{formattedNum(volume)} USDT</Text>}
        </BasicBox>
        <BasicBox $mt='1em'>
          <Text small={true} textColor='light' $pb='0.2rem'>
            <UpperCaseText>
              {getLocale('cryptoDotComWidgetPairs')}
            </UpperCaseText>
          </Text>
          {pairs.map((pair, i) => {
            const [base, quote] = pair.split('_')
            const pairName = pair.replace('_', '/')
            return (
              <ActionButton onClick={handleAssetClick.bind(this, base, quote, AssetViews.TRADE)} key={pair} small={true} inline={true} $mr={i === 0 ? 5 : 0} $mb={5}>
                {pairName}
              </ActionButton>
            )
          })}
        </BasicBox>
      </FlexItem>
    </Box>
  )
}
