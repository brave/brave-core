// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// types
import {
  MeldCryptoQuote,
  MeldServiceProvider
} from '../../../../../constants/types'

// utils
import Amount from '../../../../../utils/amount'
import { toProperCase } from '../../../../../utils/string-utils'

// styles
import {
  ProviderImage,
  ProviderName,
  StyledWrapper,
  Estimate,
  PaymentMethodsWrapper,
  PaymentMethodIcon,
  CaratIcon,
  QuoteDetailsWrapper,
  QuoteDetailsRow,
  QuoteDetailsLabel,
  QuoteDetailsValue,
  Divider,
  QuoteTotal,
  BuyButton,
  BestOptionLabel
} from './buy_quote.style'
import { Column, Row } from '../../../../../components/shared/style'

interface BuyQuoteProps {
  quote: MeldCryptoQuote
  serviceProviders: MeldServiceProvider[]
  isBestOption?: boolean
  isCreatingWidget: boolean
  onBuy: (quote: MeldCryptoQuote) => void
}

export const BuyQuote = ({
  quote,
  serviceProviders,
  isBestOption,
  isCreatingWidget,
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

  // state
  const [isOpen, setIsOpen] = React.useState(true)

  // computed
  const formattedSourceAmount = new Amount(sourceAmount ?? '').formatAsFiat(
    sourceCurrencyCode,
    2
  )
  const formattedCryptoAmount = new Amount(
    destinationAmount ?? ''
  ).formatAsAsset(5, destinationCurrencyCode)

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
        alignItems='flex-start'
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
              BEST OPTION
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
          <QuoteDetailsWrapper>
            <QuoteDetailsRow>
              <QuoteDetailsLabel>Exchange rate with fees</QuoteDetailsLabel>
              <QuoteDetailsValue>
                ≈ {formattedExchangeRate} {sourceCurrencyCode} /{' '}
                {destinationCurrencyCode}
              </QuoteDetailsValue>
            </QuoteDetailsRow>
            <QuoteDetailsRow>
              <QuoteDetailsLabel>Price {sourceCurrencyCode}</QuoteDetailsLabel>
              <QuoteDetailsValue>
                ≈ {amountWithoutFees} {sourceCurrencyCode}
              </QuoteDetailsValue>
            </QuoteDetailsRow>
            <QuoteDetailsRow>
              <QuoteDetailsLabel>Fees</QuoteDetailsLabel>
              <QuoteDetailsValue>
                {formattedTotalFee} {sourceCurrencyCode}
              </QuoteDetailsValue>
            </QuoteDetailsRow>
            <Divider />
            <QuoteDetailsRow>
              <QuoteTotal>Total</QuoteTotal>
              <QuoteTotal>
                {formattedSourceAmount} {sourceCurrencyCode}
              </QuoteTotal>
            </QuoteDetailsRow>
          </QuoteDetailsWrapper>
          <BuyButton
            isLoading={isCreatingWidget}
            onClick={() => onBuy(quote)}
          >
            Buy with {formattedProviderName}
            <div slot='icon-after'>
              <Icon name='launch' />
            </div>
          </BuyButton>
        </Column>
      ) : null}
    </StyledWrapper>
  )
}
