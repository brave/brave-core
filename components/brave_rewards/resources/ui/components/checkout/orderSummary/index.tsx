/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'

import {
  Container,
  BatAmount,
  BatSymbol,
  ExchangeAmount,
  OrderTotal
} from './style'

interface OrderSummaryProps {
  orderTotal: string
  orderTotalConverted: string
}

export function OrderSummary (props: OrderSummaryProps) {
  const locale = React.useContext(LocaleContext)

  return (
    <Container>
        <OrderTotal>{locale.get('orderTotal')}</OrderTotal>
          <ExchangeAmount>{props.orderTotalConverted}</ExchangeAmount>
          <BatAmount>
            {props.orderTotal}
            <BatSymbol>{locale.get('bat')}</BatSymbol>
          </BatAmount>
    </Container>
  )
}
