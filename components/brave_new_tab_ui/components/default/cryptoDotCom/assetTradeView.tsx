// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratLeftIcon } from 'brave-ui/components/icons'

import { currencyNames } from './data'
import {
  formattedNum,
  decimalizeCurrency,
  renderIconAsset
} from './utils'
import { getLocale } from '../../../../common/locale'

import {
  ActionButton,
  AmountInputField,
  BackArrow,
  BasicBox,
  Box,
  ButtonGroup,
  FlexItem,
  InvalidCopy,
  InvalidTitle,
  InvalidWrapper,
  PlainButton,
  SmallButton,
  StyledParty,
  Text
} from './style'
import { AssetViews } from './types'

interface Props {
  base: string
  quote: string
  availableBalanceBase: number
  availableBalanceQuote: number
  priceDecimals: string
  quantityDecimals: string
  tickerPrices: Record<string, chrome.cryptoDotCom.TickerPrice>
  handleBackClick: () => void
  handleAssetClick: (base: string, quote?: string, view?: AssetViews) => void
  createCryptoDotComMarketOrder: (order: chrome.cryptoDotCom.Order, callback: (result: chrome.cryptoDotCom.OrderResult) => void) => void
}

export default function AssetTradeView ({
  base,
  quote,
  availableBalanceBase,
  availableBalanceQuote,
  handleBackClick,
  handleAssetClick,
  createCryptoDotComMarketOrder,
  tickerPrices,
  priceDecimals,
  quantityDecimals
}: Props) {

  enum TradeModes {
    BUY = 'BUY',
    SELL = 'SELL'
  }

  const tradeModeLocaleStrings = {
    BUY: getLocale('cryptoDotComWidgetBuy'),
    SELL: getLocale('cryptoDotComWidgetSell')
  }

  enum Percentages {
    twentyFive = 25,
    fifty = 50,
    seventyFive = 75,
    onehundred = 100
  }

  const confirmDelay = 15

  const [tradeMode, setTradeMode] = React.useState(TradeModes.BUY)
  const [tradePercentage, setTradePercentage] = React.useState<number | null>(null)
  const [tradeAmount, setTradeAmount] = React.useState('')
  const [showConfirmScreen, setConfirmScreen] = React.useState(false)
  const [counter, setCounter] = React.useState(confirmDelay)
  const [tradeSuccess, setTradeSuccess] = React.useState(false)
  const [tradeFailed, setTradeFailed] = React.useState(false)
  const [tradeFailedMessage, setTradeFailedMessage] = React.useState('')

  const { price: unitPrice = 0 } = tickerPrices[`${base}_${quote}`] || {}
  const approxTotal = Number(tradeAmount) * unitPrice

  const handlePercentageClick = (percentage: number) => {
    const availableBalance =
        (tradeMode === TradeModes.BUY ? availableBalanceQuote : availableBalanceBase)
    const amount = (percentage / 100) * availableBalance
    setTradeAmount(`${amount}`)
    setTradePercentage(percentage)
  }

  const handleAmountChange = ({ target }: any) => {
    const { value } = target
    if (value === '.' || !Number.isNaN(value * 1)) {
      const available = tradeMode === TradeModes.BUY ? availableBalanceQuote
                                                     : availableBalanceBase
      // Can't put more larger amount than available.
      if (Number(available) < Number(value)) {
        return
      }
      setTradeAmount(value)
      setTradePercentage(null)
    }
  }

  const getPlaceholderText = () => tradeMode === TradeModes.BUY ? (
    getLocale('cryptoDotComWidgetTradeTo', {
      fromCurrency: quote,
      toCurrency: base
    })
  ) : (
    getLocale('cryptoDotComWidgetTradeTo', {
      fromCurrency: base,
      toCurrency: quote
    })
  )

  const makeOrderCallback = (result: chrome.cryptoDotCom.OrderResult) => {
    if (result.success) {
      setTradeSuccess(true)
    } else {
      setTradeFailed(true)
      setTradeFailedMessage(result.message)
    }
  }

  const timerRef = React.useRef<number>()
  const clearTimers = () => {
    clearInterval(timerRef.current)
    setCounter(confirmDelay)
  }

  const makeOrder = () => {
    // Call order api.
    const order = {
      'instrument_name': `${base}_${quote}`,
      'type': 'MARKET',
      'side': tradeMode
    }

    if (tradeMode === TradeModes.BUY) {
      order['notional'] = decimalizeCurrency(approxTotal.toString(), Number(priceDecimals))
    } else {
      order['quantity'] = decimalizeCurrency(tradeAmount.toString(), Number(quantityDecimals))
    }

    createCryptoDotComMarketOrder(order, makeOrderCallback)
    clearTimers()
    setConfirmScreen(false)
  }

  const cancelOrder = () => {
    clearTimers()
    setConfirmScreen(false)
  }

  React.useEffect(() => {
    if (showConfirmScreen && counter > 0) {
      setTimeout(() => {
        if (counter > 0) {
          setCounter(counter - 1)
        }
      }, 1000)
    }

    if (showConfirmScreen && counter === 0) {
      cancelOrder()
    }
  }, [counter, showConfirmScreen])

  const handlePurchaseClick = () => {
    setConfirmScreen(true)
  }

  const handleConfirmClick = () => {
    makeOrder()
  }

  // Reset amount whenever changing between buy/sell.
  const handleSetTradeMode = (mode: TradeModes) => {
    setTradeMode(mode)
    setTradeAmount('')
  }

  const handleCancelClick = () => {
    cancelOrder()
  }

  const showDepositView = () => {
    const currency = tradeMode === TradeModes.BUY ? quote : base
    handleAssetClick(currency, undefined, AssetViews.DEPOSIT)
  }

  const buyingString = getLocale('cryptoDotComWidgetBuying')
  const sellingString = getLocale('cryptoDotComWidgetSelling')

  const renderConfirmScreen = () => {
    return (
      <>
        <Box>
          <Text center={true} weight={600} $pb={15}>{getLocale('cryptoDotComWidgetConfirmOrder')}</Text>
          <BasicBox $pb={7}>
            <Text weight={600} textColor='light' $fontSize={12}>{tradeMode === TradeModes.BUY ? buyingString : sellingString}</Text>
            <Text $fontSize={16}>{tradeAmount} {base}</Text>
          </BasicBox>
          <BasicBox $pb={7}>
            <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxPrice')}</Text>
            <Text $fontSize={16}>{quote === 'USDT' ? formattedNum(unitPrice) : unitPrice} {base}/{quote}</Text>
          </BasicBox>
          <BasicBox $pb={7}>
            {tradeMode === TradeModes.BUY ? (
              <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxTotalSpent')}</Text>
            ) : (
              <Text weight={600} textColor='light' $fontSize={12}>*{getLocale('cryptoDotComWidgetApproxTotalReceived')}</Text>
            )}
            <Text $fontSize={16}>{quote === 'USDT' ? formattedNum(approxTotal) : approxTotal} {quote}</Text>
          </BasicBox>
          <Text textColor='light' $fontSize={12}>* {getLocale('cryptoDotComWidgetApproxFootnote')}</Text>
        </Box>
        <BasicBox $pt={15}>
          <ActionButton onClick={handleConfirmClick}>{getLocale('cryptoDotComWidgetConfirm', { counter: counter.toString() })} </ActionButton>
          <PlainButton $pb={5} onClick={handleCancelClick} $pt={10} $m='0 auto' textColor='light'>{getLocale('cryptoDotComWidgetCancel')}</PlainButton>
        </BasicBox>
      </>
    )
  }

  const finishTrade = () => {
    setTradeAmount('')
    setTradeSuccess(false)
    setTradeFailed(false)
  }

  const renderTradeSuccess = () => {
    const amount = decimalizeCurrency(tradeAmount.toString(), Number(quantityDecimals))
    const actionLabel = tradeMode === TradeModes.BUY ? 'cryptoDotComWidgetBought' : 'cryptoDotComWidgetSold'

    return (
      <InvalidWrapper>
        <StyledParty>
          🎉
        </StyledParty>
        <InvalidTitle>
          {`${getLocale(actionLabel, { amount: amount.toString(), currency: base })}`}
        </InvalidTitle>
        <SmallButton onClick={finishTrade}>
          {getLocale('cryptoDotComWidgetContinue')}
        </SmallButton>
      </InvalidWrapper>
    )
  }

  const renderTradeFailed = () => {
    const errorMessage = tradeFailedMessage || getLocale('cryptoDotComWidgetError')

    return (
      <InvalidWrapper>
        <InvalidTitle>
          {getLocale('cryptoDotComWidgetFailedTrade')}
        </InvalidTitle>
        <InvalidCopy>
          {errorMessage}
        </InvalidCopy>
        <SmallButton onClick={finishTrade}>
          {getLocale('cryptoDotComWidgetContinue')}
        </SmallButton>
      </InvalidWrapper>
    )
  }

  if (showConfirmScreen) {
    return renderConfirmScreen()
  }

  if (tradeSuccess) {
    return renderTradeSuccess()
  }

  if (tradeFailed) {
    return renderTradeFailed()
  }

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
          {renderIconAsset(base.toLowerCase())}
        </FlexItem>
        <FlexItem flex={1}>
          <Text>{base}</Text>
          <Text small={true} textColor='light'>
            {currencyNames[base]}
          </Text>
        </FlexItem>
        <FlexItem $pl={5}>
          <ButtonGroup>
            <PlainButton
              onClick={handleSetTradeMode.bind(this, TradeModes.BUY)}
              inButtonGroup={true}
              textColor='green'
            >
              Buy
            </PlainButton>
            <PlainButton
              onClick={handleSetTradeMode.bind(this, TradeModes.SELL)}
              inButtonGroup={true}
              textColor='red'
            >
              Sell
            </PlainButton>
          </ButtonGroup>
        </FlexItem>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
        hasBorder={true}
      >
        {tradeMode === TradeModes.BUY ? (
          <Text $mt={15} center={true}>{availableBalanceQuote} {quote} {getLocale('cryptoDotComWidgetAvailable')}</Text>
        ) : (
          <Text $mt={15} center={true}>{availableBalanceBase} {base} {getLocale('cryptoDotComWidgetAvailable')}</Text>
        )}
        <AmountInputField
          $mt={10}
          $mb={10}
          placeholder={getPlaceholderText()}
          onChange={handleAmountChange}
          value={tradeAmount}
        />
        <BasicBox isFlex={true} justify='center' $mb={13.5}>
          {Object.values(Percentages).map(percentage => {
            return (typeof percentage === 'number') && (
              <PlainButton
                key={percentage}
                weight={500}
                textColor={tradePercentage === percentage ? 'white' : 'light'}
                onClick={handlePercentageClick.bind(this, percentage)}
              >
                {percentage}%
              </PlainButton>
            )
          })}
        </BasicBox>
      </FlexItem>
      <FlexItem
        hasPadding={true}
        isFullWidth={true}
      >
        <ActionButton
          onClick={handlePurchaseClick}
          disabled={!tradeAmount}
          textOpacity={tradeAmount ? 1 : 0.6}
          $bg={tradeMode === TradeModes.BUY ? 'green' : 'red-bright'}
          upperCase={false}
        >
          {tradeModeLocaleStrings[tradeMode]} {base}
        </ActionButton>
        <ActionButton
          onClick={showDepositView}
          small={true}
          light={true}
          $mt={10}
        >
          {getLocale('cryptoDotComWidgetDepositMore', { currency: tradeMode === TradeModes.BUY ? quote : base })}
        </ActionButton>
      </FlexItem>
    </Box>
  )
}
