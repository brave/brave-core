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
  CaratIcon
} from './buy_quote.style'
import { Column, Row } from '../../../../../components/shared/style'

interface BuyQuoteProps {}

export const BuyQuote = (props: BuyQuoteProps) => {
  const [isOpen, setIsOpen] = React.useState(false)

  return (
    <StyledWrapper>
      <Row
        justifyContent='space-between'
        alignItems='flex-start'
        width='100%'
        onClick={() => setIsOpen(!isOpen)}
      >
        <Row 
        justifyContent='flex-start'
        alignItems='center'>
          <ProviderImage src='https://via.placeholder.com/40' />

          <Column>
            <ProviderName>Transak</ProviderName>
            <Estimate>€100.00 = ~0.0609 ETH</Estimate>
          </Column>
        </Row>
        <Row gap='8px'>
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
    </StyledWrapper>
  )
}
