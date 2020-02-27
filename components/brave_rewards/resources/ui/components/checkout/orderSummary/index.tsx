/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../localeContext'

import {
  Container,
  Description,
  BatAmount,
  BatSymbol,
  ExchangeAmount,
  StyledTable,
  StyledTableHeader,
  StyledTableCell
} from './style'

interface OrderSummaryProps {
  description: string
  orderTotal: string
  orderTotalConverted: string
}

export function OrderSummary (props: OrderSummaryProps) {
  const locale = React.useContext(LocaleContext)
  return (
    <Container>
      <StyledTable>
        <thead>
          <tr>
            <StyledTableHeader>{locale.get('itemSelected')}</StyledTableHeader>
            <StyledTableHeader>{locale.get('orderTotal')}</StyledTableHeader>
          </tr>
        </thead>
        <tbody>
          <tr>
            <StyledTableCell>
              <Description>{props.description}</Description>
            </StyledTableCell>
            <StyledTableCell>
              <BatAmount>
                {props.orderTotal}
                <BatSymbol>{locale.get('bat')}</BatSymbol>
              </BatAmount>
              <ExchangeAmount>{props.orderTotalConverted}</ExchangeAmount>
            </StyledTableCell>
          </tr>
        </tbody>
      </StyledTable>
    </Container>
  )
}
