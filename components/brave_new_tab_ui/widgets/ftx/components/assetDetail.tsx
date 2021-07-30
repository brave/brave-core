// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratLeftIcon, EmoteSadIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'
import Loading from '../../../components/loading'
import { currencyNames } from '../../shared/data'
import * as S from '../../shared/styles'
import { Chart } from '../../shared'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'
import IconAsset from '../../shared/iconAsset'
import getFormattedPrice from '../../shared/getFormattedPrice'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
}

export default function AssetDetail (props: Props) {
  const assetDetail = props.ftx.assetDetail
  // Sanity check
  if (!assetDetail) {
    return null
  }

  const formattedPrice = React.useMemo(() => {
    return getFormattedPrice(assetDetail.marketData?.price || 0)
  }, [assetDetail.marketData?.price])

  const formattedVolume = React.useMemo(() => {
    return getFormattedPrice(assetDetail.marketData?.volumeDay || 0)
  }, [assetDetail.marketData?.volumeDay])

  const currency = assetDetail.currencyName
  const percentChange = assetDetail.marketData?.percentChangeDay
  const chartData = assetDetail.chartData
  const chartDataError = chartData && typeof chartData === 'string'
  const waitingForChartData = !chartData
  const chartHeight = 100
  const chartWidth = 309
  return (
    <S.Box hasPadding={false}>
      <S.FlexItem
        hasPadding={true}
        isFlex={true}
        isFullWidth={true}
        hasBorder={true}
      >
        <S.FlexItem>
          <S.BackArrow>
            <CaratLeftIcon onClick={props.actions.hideAssetDetail} />
          </S.BackArrow>
        </S.FlexItem>
        <S.FlexItem $pr={5}>
          <IconAsset iconKey={currency.toLowerCase()} />
        </S.FlexItem>
        <S.FlexItem flex={1}>
          <S.Text>{currency}</S.Text>
          <S.Text small={true} textColor='light'>
            {currencyNames[currency]}
          </S.Text>
        </S.FlexItem>
      </S.FlexItem>
      <S.FlexItem
        hasPadding={true}
        isFullWidth={true}
        hasBorder={true}
      >
        {(formattedPrice) && <S.Text
          inline={true}
          large={true}
          weight={500}
          $mr='0.5rem'
        >
          {formattedPrice} USDT
        </S.Text>}
        {(percentChange) && <S.Text inline={true} textColor={percentChange > 0 ? 'green' : 'red'}>{percentChange}%</S.Text>}
        <S.BasicBox $h={chartHeight}>
          {chartData && typeof chartData !== 'string' &&
          <Chart width={chartWidth} height={chartHeight} data={chartData} />
          }
          {waitingForChartData &&
          <Loading />
          }
          {chartDataError &&
          <EmoteSadIcon />
          }
        </S.BasicBox>
        <S.Text small={true} textColor='xlight'>
          {getLocale('ftxGraphLabel')}
        </S.Text>
      </S.FlexItem>
      <S.FlexItem
        hasPadding={true}
        isFullWidth={true}
      >
        <S.BasicBox $mt='0.2em'>
          <S.Text small={true} textColor='light' $pb='0.2rem'>
            <S.UpperCaseText>
              {getLocale('ftxVolumeLabel')}
            </S.UpperCaseText>
          </S.Text>
          {formattedVolume && <S.Text weight={500}>{formattedVolume} USDT</S.Text>}
        </S.BasicBox>
      </S.FlexItem>
    </S.Box>
  )
}
