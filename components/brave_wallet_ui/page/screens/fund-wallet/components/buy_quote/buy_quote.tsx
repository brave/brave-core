// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Types
import {
  MeldCryptoCurrency,
  MeldCryptoQuote,
  MeldServiceProvider
} from '../../../../../constants/types'

// Utils
import Amount from '../../../../../utils/amount'
import { toProperCase } from '../../../../../utils/string-utils'
import { getLocale } from '../../../../../../common/locale'
import { getAssetSymbol } from '../../../../../utils/meld_utils'

// Styled Components
import {
  ProviderImage,
  ProviderName,
  StyledWrapper,
  Estimate,
  PaymentMethodsWrapper,
  PaymentMethodIcon,
  CaratIcon,
  QuoteDetailsWrapper,
  QuoteDetailsLabel,
  QuoteDetailsValue,
  Divider,
  QuoteTotal,
  BuyButton,
  BestOptionLabel,
  WrapperForPadding
} from './buy_quote.style'
import { Column, Row } from '../../../../../components/shared/style'

interface BuyQuoteProps {
  quote: MeldCryptoQuote
  serviceProviders: MeldServiceProvider[]
  isOpenOverride?: boolean
  isBestOption?: boolean
  isCreatingWidget: boolean
  selectedAsset?: MeldCryptoCurrency
  onBuy: (quote: MeldCryptoQuote) => void
}

export const BuyQuote = ({
  quote,
  serviceProviders,
  isBestOption,
  isCreatingWidget,
  selectedAsset,
  isOpenOverride,
  onBuy
}: BuyQuoteProps) => {
  const {
    serviceProvider,
    sourceCurrencyCode,
    sourceAmount,
    destinationAmount,
    destinationCurrencyCode,
    exchangeRate,
    sourceAmountWithoutFee,
    totalFee,
    paymentMethod
  } = quote

  // State
  const [isOpen, setIsOpen] = React.useState(isOpenOverride ?? false)

  // Computed
  const formattedSourceAmount = new Amount(sourceAmount ?? '').formatAsFiat(
    sourceCurrencyCode,
    2
  )

  const assetsSymbol = selectedAsset
    ? getAssetSymbol(selectedAsset)
    : destinationCurrencyCode

  const formattedCryptoAmount = new Amount(
    destinationAmount ?? ''
  ).formatAsAsset(5, assetsSymbol)

  const formattedExchangeRate = new Amount(exchangeRate ?? '').formatAsFiat(
    '',
    2
  )
  const amountWithoutFees = new Amount(
    sourceAmountWithoutFee ?? ''
  ).formatAsFiat('', 2)
  const formattedTotalFee = new Amount(totalFee ?? '').formatAsFiat('', 2)
  const [isCreditCardSupported, isDeditCardSupported] = [
    paymentMethod?.includes('CREDIT'),
    paymentMethod?.includes('DEBIT')
  ]
  const formattedProviderName = toProperCase(serviceProvider ?? '')
  const quoteServiceProvider = serviceProviders.find(
    (provider) => provider.serviceProvider === serviceProvider
  )
  const providerImageUrl = window.matchMedia('(prefers-color-scheme: dark)')
    .matches
    ? quoteServiceProvider?.logoImages?.darkShortUrl
    : quoteServiceProvider?.logoImages?.lightShortUrl

  return (
    <StyledWrapper isOpen={isOpen}>
      <Row
        justifyContent='space-between'
        width='100%'
        onClick={() => setIsOpen(!isOpen)}
      >
        <Row
          justifyContent='flex-start'
          alignItems='center'
        >
          {providerImageUrl ? (
            <ProviderImage src={`chrome://image?${providerImageUrl}`} />
          ) : null}

          <Column alignItems='flex-start'>
            <ProviderName>{formattedProviderName}</ProviderName>
            <Estimate>
              {formattedSourceAmount} = ~{formattedCryptoAmount}
            </Estimate>
          </Column>
        </Row>
        <Row
          gap='8px'
          justifyContent='flex-end'
        >
          {isBestOption ? (
            <BestOptionLabel>
              <div slot='icon-before'>
                <Icon name='thumb-up' />
              </div>
              {getLocale('braveWalletBestOption')}
            </BestOptionLabel>
          ) : null}
          <PaymentMethodsWrapper>
            {isDeditCardSupported ? <PaymentMethodIcon name='bank' /> : null}
            {isCreditCardSupported ? (
              <PaymentMethodIcon name='credit-card' />
            ) : null}
          </PaymentMethodsWrapper>
          <CaratIcon
            isOpen={isOpen}
            name='carat-down'
          />
        </Row>
      </Row>
      {isOpen ? (
        <Column
          width='100%'
          gap='8px'
        >
          <WrapperForPadding fullWidth={true}>
            <QuoteDetailsWrapper
              fullWidth={true}
              gap='8px'
            >
              <Row
                justifyContent='space-between'
                gap='8px'
              >
                <QuoteDetailsLabel>
                  {getLocale('braveWalletExchangeRateWithFees')}
                </QuoteDetailsLabel>
                <QuoteDetailsValue>
                  ≈ {formattedExchangeRate} {sourceCurrencyCode} /{' '}
                  {assetsSymbol}
                </QuoteDetailsValue>
              </Row>
              <Row
                justifyContent='space-between'
                gap='8px'
              >
                <QuoteDetailsLabel>
                  {getLocale('braveWalletPriceCurrency').replace(
                    '$1',
                    sourceCurrencyCode ?? ''
                  )}
                </QuoteDetailsLabel>
                <QuoteDetailsValue>
                  ≈ {amountWithoutFees} {sourceCurrencyCode}
                </QuoteDetailsValue>
              </Row>
              <Row
                justifyContent='space-between'
                gap='8px'
              >
                <QuoteDetailsLabel>
                  {getLocale('braveWalletFees')}
                </QuoteDetailsLabel>
                <QuoteDetailsValue>
                  {formattedTotalFee} {sourceCurrencyCode}
                </QuoteDetailsValue>
              </Row>
              <Divider />
              <Row
                justifyContent='space-between'
                gap='8px'
              >
                <QuoteTotal>
                  {getLocale('braveWalletConfirmTransactionTotal')}
                </QuoteTotal>
                <QuoteTotal>
                  {formattedSourceAmount} {sourceCurrencyCode}
                </QuoteTotal>
              </Row>
            </QuoteDetailsWrapper>
          </WrapperForPadding>
          <BuyButton
            isLoading={isCreatingWidget}
            onClick={() => onBuy(quote)}
          >
            {getLocale('braveWalletBuyWithProvider').replace(
              '$1',
              formattedProviderName
            )}
            <div slot='icon-after'>
              <Icon name='launch' />
            </div>
          </BuyButton>
        </Column>
      ) : null}
    </StyledWrapper>
  )
}
