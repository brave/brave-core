// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocaleWithTag } from '../../../../common/locale'
import * as S from '../../shared/styles'
import IconAsset from '../../shared/iconAsset'
import { currencyNames } from '../../shared/data'
import getFormattedPrice from '../../shared/getFormattedPrice'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
}

export default function Markets (props: Props) {
  const moreMarketsTextParts = React.useMemo(() => {
    return getLocaleWithTag('ftxMoreMarketsLink')
  }, [])

  const handleAssetDetailClick = React.useCallback((symbol) => {
    props.actions.showAssetDetail({ symbol })
  }, [ props.actions.showAssetDetail ])

  return (
    <>
      <S.List $mt={10}>
        {props.ftx.marketData.map(market => {
          const { symbol, price, percentChangeDay } = market
          const currencyName = currencyNames[symbol]
          return (
            <S.ListItem key={symbol} isFlex={true} onClick={handleAssetDetailClick.bind(undefined, symbol)} $height={48}>
              <S.FlexItem $pl={5} $pr={5}>
                <IconAsset iconKey={symbol.toLowerCase()} />
              </S.FlexItem>
              <S.FlexItem>
                <S.Text>{symbol}</S.Text>
                {currencyName &&
                <S.Text small={true} textColor='light'>{currencyNames[symbol]}</S.Text>
                }
              </S.FlexItem>
              <S.FlexItem textAlign='right' flex={1}>
                <S.Text>{getFormattedPrice(price)}</S.Text>
                <S.Text textColor={percentChangeDay > 0 ? 'green' : 'red'}>{percentChangeDay}%</S.Text>
              </S.FlexItem>
            </S.ListItem>
          )
        })}
      </S.List>
      <S.Text $mt={13} center={true}>
        {moreMarketsTextParts.beforeTag}
        <S.PlainAnchor href={`https://${props.ftx.ftxHost}/markets`}>
          {props.ftx.ftxHost}
        </S.PlainAnchor>
        {moreMarketsTextParts.afterTag}
      </S.Text>
    </>
  )
}
