// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

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
import Icon from '@brave/leo/react/icon'

interface BuyQuoteProps {}

export const BuyQuote = (props: BuyQuoteProps) => {
  const [isOpen, setIsOpen] = React.useState(true)

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
          <ProviderImage src='https://via.placeholder.com/40' />

          <Column alignItems='flex-start'>
            <ProviderName>Transak</ProviderName>
            <Estimate>€100.00 = ~0.0609 ETH</Estimate>
          </Column>
        </Row>
        <Row
          gap='8px'
          justifyContent='flex-end'
        >
          <BestOptionLabel>
            <div slot='icon-before'>
              <Icon name='thumb-up' />
            </div>
            BEST OPEN
          </BestOptionLabel>
          <PaymentMethodsWrapper>
            <PaymentMethodIcon name='bank' />
            <PaymentMethodIcon name='credit-card' />
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
              <QuoteDetailsValue>≈ 1,601.78 EUR / ETH</QuoteDetailsValue>
            </QuoteDetailsRow>
            <QuoteDetailsRow>
              <QuoteDetailsLabel>Price EUR</QuoteDetailsLabel>
              <QuoteDetailsValue>≈ 95.00 EUR</QuoteDetailsValue>
            </QuoteDetailsRow>
            <QuoteDetailsRow>
              <QuoteDetailsLabel>Fees</QuoteDetailsLabel>
              <QuoteDetailsValue>≈ 5.00 EUR</QuoteDetailsValue>
            </QuoteDetailsRow>
            <Divider />
            <QuoteDetailsRow>
              <QuoteTotal>Total</QuoteTotal>
              <QuoteTotal>100.00EUR</QuoteTotal>
            </QuoteDetailsRow>
          </QuoteDetailsWrapper>
          <BuyButton>
            Buy with Transak
            <div slot='icon-after'>
              <Icon name='launch' />
            </div>
          </BuyButton>
        </Column>
      ) : null}
    </StyledWrapper>
  )
}
