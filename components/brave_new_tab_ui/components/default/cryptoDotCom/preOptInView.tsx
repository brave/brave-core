// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  ActionButton,
  Box,
  FlexItem,
  PlainButton,
  Text
} from './style'

import {
  formattedNum,
  getPercentColor,
  transformLosersGainers
} from './utils'

import IconAsset from '../../../widgets/shared/iconAsset'
import { currencyNames } from '../../../widgets/shared/data'
import { getLocale } from '../../../../common/locale'
interface Props {
  optInBTCPrice: boolean
  losersGainers: Record<string, chrome.cryptoDotCom.AssetRanking[]>
  tickerPrices: Record<string, chrome.cryptoDotCom.TickerPrice>
  onBTCPriceOptedIn: () => void
  getCryptoDotComClientUrl: (callback: (clientAuthUrl: string) => void) => void
}

export default function PreOptInView ({
  optInBTCPrice,
  tickerPrices,
  losersGainers,
  onBTCPriceOptedIn,
  getCryptoDotComClientUrl
}: Props) {
  const currency = 'BTC'
  const { price = null } = tickerPrices[`${currency}_USDT`] || {}
  const transformedLosersGainers = transformLosersGainers(losersGainers || {})
  const { percentChange = null } = transformedLosersGainers[currency] || {}

  const [clientAuthUrl, setClientAuthUrl] = React.useState('')

  const onClickConnectToCryptoDotCom = () => {
    window.open(clientAuthUrl, '_self', 'noopener')
  }

  getCryptoDotComClientUrl((clientAuthUrl: string) => {
    setClientAuthUrl(clientAuthUrl)
  })

  return (
    <>
      <Box isFlex={true} $height={48}>
        <FlexItem $pr={5}>
          <IconAsset iconKey={currency.toLowerCase()} />
        </FlexItem>
        <FlexItem>
            <Text>{currency}</Text>
            <Text small={true} textColor='light'>{currencyNames[currency]}</Text>
        </FlexItem>
        <FlexItem textAlign='right' flex={1}>
          {optInBTCPrice ? (
            <>
              {(price !== null) && <Text>{formattedNum(price)}</Text>}
              {(percentChange !== null) && <Text textColor={getPercentColor(percentChange)}>{percentChange}%</Text>}
            </>
          ) : (
            <PlainButton onClick={onBTCPriceOptedIn} textColor='green' inline={true}>
              {getLocale('cryptoDotComWidgetShowPrice')}
            </PlainButton>
          )}
        </FlexItem>
      </Box>
      <Text $pt='1em' $fontSize={14}>
        {getLocale('cryptoDotComWidgetCopyOne')}
      </Text>
      <ActionButton onClick={onClickConnectToCryptoDotCom} $mt={10} $mb={15} disabled={clientAuthUrl === ''}>
        {getLocale('cryptoDotComWidgetConnect')}
      </ActionButton>
    </>
  )
}
