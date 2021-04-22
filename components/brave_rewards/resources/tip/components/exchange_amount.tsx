/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  amount: number
  rate: number
}

export function getExchangeCurrency () {
  return 'USD'
}

export function ExchangeAmount (props: Props) {
  const exchangeAmount = props.amount * props.rate
  return (
    <>
      <span className='amount'>{exchangeAmount.toFixed(2)}</span>&nbsp;
      <span className='currency'>{getExchangeCurrency()}</span>
    </>
  )
}
